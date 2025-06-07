#include <algorithm>
#include <cstdlib>
#include <finch/core/logging.hpp>
#include <finch/core/otel_integration.hpp>
#include <iomanip>
#include <random>
#include <sstream>

namespace finch {

// Static member definitions
bool OtelIntegration::initialized_ = false;
OtelConfig OtelIntegration::config_;
std::string OtelIntegration::trace_endpoint_;
std::string OtelIntegration::metrics_endpoint_;
std::string OtelIntegration::logs_endpoint_;

bool OtelIntegration::initialize(const OtelConfig& config) {
    config_ = config;

    if (!config.enabled) {
        initialized_ = false;
        return true;
    }

    // Parse endpoint and construct specific endpoints
    std::string base_url = config.endpoint;
    if (base_url.back() == '/') {
        base_url.pop_back();
    }

    logs_endpoint_ = base_url + "/v1/logs";
    metrics_endpoint_ = base_url + "/v1/metrics";
    trace_endpoint_ = base_url + "/v1/traces";

    // Set resource attributes
    config_.resource_attributes["service.name"] = config.service_name;
    config_.resource_attributes["service.version"] = config.service_version;

    initialized_ = true;
    return true;
}

void OtelIntegration::shutdown() {
    if (initialized_) {
        initialized_ = false;
    }
}

bool OtelIntegration::export_log(const LogEntry& entry) {
    if (!is_enabled()) {
        return false;
    }

    try {
        std::string payload = build_log_payload(entry);
        return send_http_request(logs_endpoint_, payload);
    } catch (const std::exception&) {
        return false;
    }
}

bool OtelIntegration::record_metric(const std::string& name, double value,
                                    const std::map<std::string, std::string>& labels,
                                    const std::string& unit) {
    if (!is_enabled() || !config_.metrics.enabled) {
        return false;
    }

    try {
        std::string payload = build_metric_payload(name, value, labels, unit, "gauge");
        return send_http_request(metrics_endpoint_, payload);
    } catch (const std::exception&) {
        return false;
    }
}

bool OtelIntegration::record_counter(const std::string& name, int64_t value,
                                     const std::map<std::string, std::string>& labels) {
    if (!is_enabled() || !config_.metrics.enabled) {
        return false;
    }

    try {
        std::string payload =
            build_metric_payload(name, static_cast<double>(value), labels, "", "counter");
        return send_http_request(metrics_endpoint_, payload);
    } catch (const std::exception&) {
        return false;
    }
}

bool OtelIntegration::record_histogram(const std::string& name, double value,
                                       const std::map<std::string, std::string>& labels,
                                       const std::string& unit) {
    if (!is_enabled() || !config_.metrics.enabled || !config_.metrics.include_histograms) {
        return false;
    }

    try {
        std::string payload = build_metric_payload(name, value, labels, unit, "histogram");
        return send_http_request(metrics_endpoint_, payload);
    } catch (const std::exception&) {
        return false;
    }
}

std::unique_ptr<Span>
OtelIntegration::start_span(const std::string& name,
                            const std::map<std::string, std::string>& attributes) {
    if (!is_enabled() || !config_.traces.enabled) {
        return nullptr;
    }

    return std::make_unique<OtelSpan>(name, attributes);
}

bool OtelIntegration::send_telemetry_json(const std::string& endpoint_path,
                                          const std::string& json_data) {
    if (!is_enabled()) {
        return false;
    }

    std::string full_url = config_.endpoint;
    if (full_url.back() == '/') {
        full_url.pop_back();
    }
    full_url += endpoint_path;

    return send_http_request(full_url, json_data);
}

std::string OtelIntegration::build_log_payload(const LogEntry& entry) {
    // Simplified JSON construction to avoid formatter issues
    std::ostringstream json;
    json << "{\"resourceLogs\":[{";
    json << "\"resource\":" << build_resource_attributes() << ",";
    json << "\"scopeLogs\":[{";
    json << "\"scope\":{\"name\":\"finch-buck2\"},";
    json << "\"logRecords\":[{";
    json << "\"timeUnixNano\":" << get_timestamp_nanos() << ",";
    json << "\"severityText\":\"" << entry.level << "\",";
    json << "\"body\":{\"stringValue\":\"" << entry.message << "\"}";

    if (!entry.module.empty() || !entry.attributes.empty()) {
        json << ",\"attributes\":[";
        bool first = true;

        if (!entry.module.empty()) {
            json << "{\"key\":\"module\",\"value\":{\"stringValue\":\"" << entry.module << "\"}}";
            first = false;
        }

        for (const auto& attr : entry.attributes) {
            if (!first)
                json << ",";
            json << "{\"key\":\"" << attr.first << "\",\"value\":{\"stringValue\":\"" << attr.second
                 << "\"}}";
            first = false;
        }
        json << "]";
    }

    json << ",\"traceId\":\"" << generate_trace_id() << "\"";
    json << ",\"spanId\":\"" << generate_span_id() << "\"";
    json << "}]}]}]}";

    return json.str();
}

std::string OtelIntegration::build_metric_payload(const std::string& name, double value,
                                                  const std::map<std::string, std::string>& labels,
                                                  const std::string& unit,
                                                  const std::string& metric_type) {
    std::ostringstream json;
    json << "{\"resourceMetrics\":[{";
    json << "\"resource\":" << build_resource_attributes() << ",";
    json << "\"scopeMetrics\":[{";
    json << "\"scope\":{\"name\":\"finch-buck2\"},";
    json << "\"metrics\":[{";
    json << "\"name\":\"" << name << "\"";

    if (!unit.empty()) {
        json << ",\"unit\":\"" << unit << "\"";
    }

    if (metric_type == "counter") {
        json << ",\"sum\":{\"dataPoints\":[{";
        json << "\"timeUnixNano\":" << get_timestamp_nanos() << ",";
        json << "\"asInt\":" << static_cast<int64_t>(value);
    } else if (metric_type == "histogram") {
        json << ",\"histogram\":{\"dataPoints\":[{";
        json << "\"timeUnixNano\":" << get_timestamp_nanos() << ",";
        json << "\"count\":1,\"sum\":" << value << ",\"bucketCounts\":[1]";
    } else {
        json << ",\"gauge\":{\"dataPoints\":[{";
        json << "\"timeUnixNano\":" << get_timestamp_nanos() << ",";
        json << "\"asDouble\":" << value;
    }

    if (!labels.empty()) {
        json << ",\"attributes\":[";
        bool first = true;
        for (const auto& label : labels) {
            if (!first)
                json << ",";
            json << "{\"key\":\"" << label.first << "\",\"value\":{\"stringValue\":\""
                 << label.second << "\"}}";
            first = false;
        }
        json << "]";
    }

    json << "}]";
    if (metric_type == "histogram") {
        json << ",\"aggregationTemporality\":2";
    }
    json << "}]}]}]}";

    return json.str();
}

std::string
OtelIntegration::build_trace_payload(const std::string& span_name,
                                     const std::map<std::string, std::string>& attributes) {
    std::ostringstream json;
    json << "{\"resourceSpans\":[{";
    json << "\"resource\":" << build_resource_attributes() << ",";
    json << "\"scopeSpans\":[{";
    json << "\"scope\":{\"name\":\"finch-buck2\"},";
    json << "\"spans\":[{";
    json << "\"traceId\":\"" << generate_trace_id() << "\",";
    json << "\"spanId\":\"" << generate_span_id() << "\",";
    json << "\"name\":\"" << span_name << "\",";
    json << "\"kind\":1,";
    json << "\"startTimeUnixNano\":" << get_timestamp_nanos() << ",";
    json << "\"endTimeUnixNano\":" << get_timestamp_nanos();

    if (!attributes.empty()) {
        json << ",\"attributes\":[";
        bool first = true;
        for (const auto& attr : attributes) {
            if (!first)
                json << ",";
            json << "{\"key\":\"" << attr.first << "\",\"value\":{\"stringValue\":\"" << attr.second
                 << "\"}}";
            first = false;
        }
        json << "]";
    }

    json << "}]}]}]}";
    return json.str();
}

bool OtelIntegration::send_http_request(const std::string&, const std::string&) {
    // Simplified implementation - just return false for now
    // In a real implementation, this would use a proper HTTP client
    return false;
}

std::string OtelIntegration::generate_trace_id() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    static thread_local std::uniform_int_distribution<uint64_t> dis;

    uint64_t high = dis(gen);
    uint64_t low = dis(gen);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << high << std::setw(16) << low;
    return oss.str();
}

std::string OtelIntegration::generate_span_id() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    static thread_local std::uniform_int_distribution<uint64_t> dis;

    uint64_t id = dis(gen);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << id;
    return oss.str();
}

int64_t OtelIntegration::get_timestamp_nanos() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

std::string OtelIntegration::build_resource_attributes() {
    std::ostringstream json;
    json << "{\"attributes\":[";

    bool first = true;
    for (const auto& attr : config_.resource_attributes) {
        if (!first)
            json << ",";
        json << "{\"key\":\"" << attr.first << "\",\"value\":{\"stringValue\":\"" << attr.second
             << "\"}}";
        first = false;
    }

    json << "]}";
    return json.str();
}

// OtelSpan implementation
OtelSpan::OtelSpan(std::string name, std::map<std::string, std::string> initial_attributes)
    : name_(std::move(name)), trace_id_(OtelIntegration::generate_trace_id()),
      span_id_(OtelIntegration::generate_span_id()), start_time_(std::chrono::system_clock::now()),
      attributes_(std::move(initial_attributes)) {}

OtelSpan::~OtelSpan() {
    if (!ended_) {
        end();
    }
}

void OtelSpan::end() {
    if (ended_ || !OtelIntegration::is_enabled()) {
        return;
    }

    ended_ = true;

    try {
        std::string payload = OtelIntegration::build_trace_payload(name_, attributes_);
        OtelIntegration::send_http_request(OtelIntegration::get_trace_endpoint(), payload);
    } catch (const std::exception&) {
        // Ignore errors
    }
}

// StructuredLogger implementation
void StructuredLogger::log() {
    // Log normally first
    auto logger = Logger::get();
    if (logger) {
        auto level = spdlog::level::from_str(entry_.level);
        logger->log(level, entry_.message);
    }

    // Export to OTLP if enabled
    if (OtelIntegration::is_enabled()) {
        OtelIntegration::export_log(entry_);
    }
}

} // namespace finch

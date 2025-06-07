#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

namespace finch {

/// OpenTelemetry integration configuration
struct OtelConfig {
    bool enabled = false;
    std::string endpoint = "http://localhost:4318";
    bool use_grpc = false; // false = HTTP, true = gRPC
    std::string service_name = "finch-buck2";
    std::string service_version = "0.1.0";
    std::chrono::seconds export_interval{5};
    std::chrono::seconds export_timeout{30};
    std::unordered_map<std::string, std::string> resource_attributes;

    struct MetricConfig {
        bool enabled = true;
        bool include_histograms = true;
    } metrics;

    struct TraceConfig {
        bool enabled = true;
        double sample_rate = 1.0; // 1.0 = 100% sampling
    } traces;
};

/// Log entry for OTLP export
struct LogEntry {
    std::string message;
    std::string level;
    std::chrono::system_clock::time_point timestamp;
    std::string module;
    std::unordered_map<std::string, std::string> attributes;

    LogEntry(std::string msg, std::string lvl, std::string mod = "")
        : message(std::move(msg)), level(std::move(lvl)),
          timestamp(std::chrono::system_clock::now()), module(std::move(mod)) {}

    LogEntry& with_attribute(const std::string& key, const std::string& value) {
        attributes[key] = value;
        return *this;
    }
};

/// Forward declaration for span interface
class Span {
  public:
    virtual ~Span() = default;
    virtual void set_attribute(const std::string& key, const std::string& value) = 0;
    virtual void set_attribute(const std::string& key, int64_t value) = 0;
    virtual void set_attribute(const std::string& key, double value) = 0;
    virtual void set_attribute(const std::string& key, bool value) = 0;
    virtual void set_status(const std::string& status) = 0;
    virtual void end() = 0;
};

/// OpenTelemetry integration singleton
class OtelIntegration {
  private:
    static bool initialized_;
    static OtelConfig config_;
    static std::string trace_endpoint_;
    static std::string metrics_endpoint_;
    static std::string logs_endpoint_;

  public:
    /// Initialize OpenTelemetry integration
    static bool initialize(const OtelConfig& config);

    /// Shutdown OpenTelemetry integration
    static void shutdown();

    /// Check if integration is enabled
    static bool is_enabled() noexcept {
        return initialized_ && config_.enabled;
    }

    /// Get current configuration
    static const OtelConfig& get_config() noexcept {
        return config_;
    }

    /// Export log entry via OTLP
    static bool export_log(const LogEntry& entry);

    /// Record metric via OTLP
    static bool record_metric(const std::string& name, double value,
                              const std::map<std::string, std::string>& labels = {},
                              const std::string& unit = "");

    /// Record counter metric
    static bool record_counter(const std::string& name, int64_t value,
                               const std::map<std::string, std::string>& labels = {});

    /// Record histogram metric
    static bool record_histogram(const std::string& name, double value,
                                 const std::map<std::string, std::string>& labels = {},
                                 const std::string& unit = "");

    /// Start a new trace span
    static std::unique_ptr<Span>
    start_span(const std::string& name, const std::map<std::string, std::string>& attributes = {});

    /// Send custom telemetry data
    static bool send_telemetry_json(const std::string& endpoint_path, const std::string& json_data);

    /// Build OTLP trace payload
    static std::string build_trace_payload(const std::string& span_name,
                                           const std::map<std::string, std::string>& attributes);

    /// Send HTTP POST request
    static bool send_http_request(const std::string& url, const std::string& json_data);

    /// Generate trace/span IDs
    static std::string generate_trace_id();
    static std::string generate_span_id();

    /// Get access to endpoints for friend classes
    static const std::string& get_trace_endpoint() {
        return trace_endpoint_;
    }
    static const std::string& get_metrics_endpoint() {
        return metrics_endpoint_;
    }
    static const std::string& get_logs_endpoint() {
        return logs_endpoint_;
    }

  private:
    /// Build OTLP log payload
    static std::string build_log_payload(const LogEntry& entry);

    /// Build OTLP metric payload
    static std::string build_metric_payload(const std::string& name, double value,
                                            const std::map<std::string, std::string>& labels,
                                            const std::string& unit,
                                            const std::string& metric_type);

    /// Get current timestamp in nanoseconds
    static int64_t get_timestamp_nanos();

    /// Build resource attributes
    static std::string build_resource_attributes();
};

/// RAII span implementation
class OtelSpan : public Span {
  private:
    std::string name_;
    std::string trace_id_;
    std::string span_id_;
    std::chrono::system_clock::time_point start_time_;
    std::map<std::string, std::string> attributes_;
    bool ended_ = false;

  public:
    explicit OtelSpan(std::string name, std::map<std::string, std::string> initial_attributes = {});
    ~OtelSpan() override;

    void set_attribute(const std::string& key, const std::string& value) override {
        if (!ended_) {
            attributes_[key] = value;
        }
    }

    void set_attribute(const std::string& key, int64_t value) override {
        if (!ended_) {
            attributes_[key] = std::to_string(value);
        }
    }

    void set_attribute(const std::string& key, double value) override {
        if (!ended_) {
            attributes_[key] = std::to_string(value);
        }
    }

    void set_attribute(const std::string& key, bool value) override {
        if (!ended_) {
            attributes_[key] = value ? "true" : "false";
        }
    }

    void set_status(const std::string& status) override {
        set_attribute("status", status);
    }

    void end() override;

    [[nodiscard]] const std::string& trace_id() const noexcept {
        return trace_id_;
    }
    [[nodiscard]] const std::string& span_id() const noexcept {
        return span_id_;
    }
};

/// Structured logging helper with OTLP export
class StructuredLogger {
  private:
    LogEntry entry_;

  public:
    explicit StructuredLogger(std::string message, std::string level = "info",
                              std::string module = "")
        : entry_(std::move(message), std::move(level), std::move(module)) {}

    StructuredLogger& with(const std::string& key, const std::string& value) {
        entry_.with_attribute(key, value);
        return *this;
    }

    StructuredLogger& with(const std::string& key, int64_t value) {
        entry_.with_attribute(key, std::to_string(value));
        return *this;
    }

    StructuredLogger& with(const std::string& key, double value) {
        entry_.with_attribute(key, std::to_string(value));
        return *this;
    }

    StructuredLogger& with(const std::string& key, bool value) {
        entry_.with_attribute(key, value ? "true" : "false");
        return *this;
    }

    StructuredLogger& with_duration(std::chrono::milliseconds ms) {
        entry_.with_attribute("duration_ms", std::to_string(ms.count()));
        return *this;
    }

    StructuredLogger& with_error(const std::string& error_type, const std::string& error_message) {
        entry_.with_attribute("error.type", error_type);
        entry_.with_attribute("error.message", error_message);
        return *this;
    }

    void log();
};

} // namespace finch

// Convenience macros for structured logging with OTLP export
#define LOG_STRUCTURED_INFO(msg) finch::StructuredLogger(msg, "info").log()
#define LOG_STRUCTURED_ERROR(msg) finch::StructuredLogger(msg, "error").log()
#define LOG_STRUCTURED_DEBUG(msg) finch::StructuredLogger(msg, "debug").log()
#define LOG_STRUCTURED_TRACE(msg) finch::StructuredLogger(msg, "trace").log()

#define OTEL_SPAN(name) auto _otel_span = finch::OtelIntegration::start_span(name)
#define OTEL_METRIC_COUNTER(name, value, ...)                                                      \
    finch::OtelIntegration::record_counter(name, value, ##__VA_ARGS__)
#define OTEL_METRIC_HISTOGRAM(name, value, ...)                                                    \
    finch::OtelIntegration::record_histogram(name, value, ##__VA_ARGS__)

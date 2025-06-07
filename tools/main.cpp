#include <finch/cli/application.hpp>

int main(int argc, char** argv) {
    finch::cli::Application app;
    return app.run(argc, argv);
}

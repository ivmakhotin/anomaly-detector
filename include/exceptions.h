#include <exception>
#include <stdexcept>
#include <string>

class RequestError : public std::runtime_error {
  public:
    explicit RequestError(const std::string &what_arg)
        : std::runtime_error("RequestError: " + what_arg){};
};

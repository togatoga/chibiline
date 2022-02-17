#pragma once
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace chibi {
using Value = std::variant<bool, short, unsigned short, int, unsigned int, long,
                           unsigned long, long long, unsigned long long, float,
                           double, char, unsigned char, std::string>;

constexpr std::nullopt_t none = std::nullopt;

template <class T> T cast(const std::string &word) {
  T result;
  std::stringstream ss;
  if (!(ss << word && ss >> result && ss.eof())) {
    throw std::bad_cast();
  }
  return result;
}

template <> bool cast<bool>(const std::string &word) {
  bool result;
  if (word == "true") {
    return true;
  } else if (word == "false") {
    return false;
  }
  throw std::bad_cast();
}

struct ArgumentValue {
  ArgumentValue(std::optional<std::string> name,
                std::optional<std::string> description)

      : m_name(name), m_description(description) {}
  std::optional<std::string> m_name;
  std::optional<std::string> m_description;
};
struct OptionValue {
  OptionValue(const std::string &name, const std::optional<char> short_name,
              const std::optional<std::string> &description,
              const std::optional<Value> default_value)
      : m_name(name), m_short_name(short_name), m_description(description),
        m_default_value(default_value) {}
  std::string m_name;
  std::optional<char> m_short_name;
  std::optional<std::string> m_description;
  std::optional<Value> m_default_value;
};

struct FlagValue {
  FlagValue(std::string name, std::optional<char> short_name,
            std::optional<std::string> description)
      : m_name(name), m_short_name(short_name), m_description(description) {}
  std::string m_name;
  std::optional<char> m_short_name;
  std::optional<std::string> m_description;
};

struct App {

  App(const std::optional<std::string> &app_name = std::nullopt,
      const std::optional<std::string> &description = std::nullopt)
      : m_app_name(app_name), m_description(description) {

    add_flag("help", 'h', "Print help information");
  };
  // argument
  template <class T>
  void add_arg(const std::string &name,
               const std::optional<std::string> &description = std::nullopt) {
    insert_arg_name(name);
    m_arguments.push_back(ArgumentValue(name, description));
  };

  template <class T> const T get_arg(int index) {
    if (index < m_parsed_arguments.size()) {
      return cast<T>(m_parsed_arguments[index]);
    }
    throw std::runtime_error("Invalid access argument");
  };
  // option

  template <class T, class K = T>
  void add_opt(const std::string &name,
               const std::optional<char> short_name = std::nullopt,
               const std::optional<std::string> &description = std::nullopt,
               const std::optional<T> default_value = std::nullopt) {
    insert_option_names(name, short_name);
    m_options.push_back(
        OptionValue(name, short_name, description, default_value));
    // std::cerr << m_options.back().m_name << std::endl;
  }

  void insert_arg_name(const std::string &name) {
    if (name.empty()) {
      throw std::runtime_error("Empty argument name: " + name);
    }
    if (!m_arg_name_set.insert(name).second) {
      throw std::runtime_error("Multiple argument name: " + name);
    }
  }
  void insert_option_names(const std::string &name,
                           const std::optional<char> short_name) {

    if (name.empty()) {
      throw std::runtime_error("Empty option name: " + name);
    }

    if (!m_option_name_set.insert(name).second) {
      throw std::runtime_error("Multiple option name: " + name);
    }
    if (short_name.has_value() &&
        !m_option_short_name_set.insert(short_name.value()).second) {
      throw std::runtime_error("Multiple option short name: " +
                               std::string{short_name.value()});
    }
  }

  template <class T>
  const std::optional<T> get_opt(const std::string &name) const {

    for (const auto &[word, value] : m_parsed_options) {
      if (value.m_name == name) {
        T result = cast<T>(word);
        return result;
      }
    }

    for (const auto &value : m_options) {
      if (value.m_name == name) {
        if (value.m_default_value.has_value()) {
          T result = std::get<T>(value.m_default_value.value());
          return result;
        }
      }
    }
    return std::nullopt;
  }

  // flag
  void add_flag(const std::string &name,
                const std::optional<char> short_name = std::nullopt,
                const std::optional<std::string> &description = std::nullopt) {
    insert_option_names(name, short_name);
    m_flag_options.push_back(FlagValue(name, short_name, description));
  }

  bool get_flag(const std::string &name) {
    auto opt =
        std::find_if(m_parsed_flag_options.begin(), m_parsed_flag_options.end(),
                     [&](FlagValue &value) { return value.m_name == name; });
    return opt != m_parsed_flag_options.end();
  };

  void parse(int argc, const char *const *argv) {
    if (!m_app_name.has_value()) {
      m_app_name = argv[0];
    }
    std::vector<std::string> words;
    for (size_t i = 1; i < argc; i++) {
      words.push_back(argv[i]);
    }
    parse(words);
  }
  void parse(const std::string &line) {
    std::vector<std::string> words;
    std::istringstream iss(line);
    std::string item;
    while (std::getline(iss, item, ' ')) {
      if (!item.empty()) {
        words.push_back(item);
      }
    }
    parse(words);
  }
  void parse(const std::vector<std::string> &words) {
    size_t n = words.size();
    size_t pos = 0;

    while (pos < n) {
      const std::string word = words[pos];
      if (word == "-h" || word == "--help") {
        help();
        exit(0);
        pos += 1;
        continue;
      }

      if (word.rfind("--", 0) == 0) {
        // long option
        if (word.size() == 2) {
          throw std::runtime_error("Illegal option: " + word);
        }
        std::string name = word.substr(2);
        if (m_option_name_set.count(name) == 0) {
          throw std::runtime_error("Unrecognized option: " + word);
        }

        auto opt = std::find_if(
            m_options.begin(), m_options.end(),
            [&](OptionValue &value) { return value.m_name == name; });
        auto flag_opt = std::find_if(
            m_flag_options.begin(), m_flag_options.end(),
            [&](FlagValue &value) { return value.m_name == name; });
        if (opt == m_options.end() && flag_opt == m_flag_options.end()) {
          throw std::runtime_error("Unrecognized option: " + word);
        }
        if (opt != m_options.end()) {
          if (pos + 1 >= n) {
            throw std::runtime_error("Illegal option: " + word +
                                     " require a value");
          }
          const std::string next_word = words[pos + 1];
          if (next_word.rfind("-", 0) == 0) {
            throw std::runtime_error("Illegal option: " + word +
                                     " require a value");
          }
          m_parsed_options.push_back(std::make_pair(next_word, *opt));
          pos += 1;
        } else {
          m_parsed_flag_options.push_back(*flag_opt);
        }
      } else if (word.rfind("-", 0) == 0) {
        // short option
        if (word.size() == 1) {
          throw std::runtime_error("Illegal option: " + word);
        }
        char short_name = word[1];
        if (m_option_short_name_set.count(short_name) == 0) {
          throw std::runtime_error("Unrecognized option: " + word);
        }
        auto opt = std::find_if(
            m_options.begin(), m_options.end(), [&](OptionValue &value) {
              return value.m_short_name.has_value() &&
                     value.m_short_name.value() == short_name;
            });
        auto flag_opt =
            std::find_if(m_flag_options.begin(), m_flag_options.end(),
                         [&](FlagValue &value) {
                           return value.m_short_name.has_value() &&
                                  value.m_short_name.value() == short_name;
                         });
        if (opt == m_options.end() && flag_opt == m_flag_options.end()) {
          throw std::runtime_error("Unrecognized option: " + word);
        }
        if (opt != m_options.end()) {
          if (pos + 1 >= n) {
            throw std::runtime_error("Illegal option: " + word +
                                     " require a value");
          }
          const std::string next_word = words[pos + 1];
          if (next_word.rfind("-", 0) == 0) {
            throw std::runtime_error("Illegal option: " + word +
                                     " require a value");
          }
          m_parsed_options.push_back(std::make_pair(next_word, *opt));
          pos += 1;
        } else {
          m_parsed_flag_options.push_back(*flag_opt);
        }

      } else {
        m_parsed_arguments.push_back(word);
      }
      pos += 1;
    }
  }

  void help() {
    // description
    if (m_description.has_value()) {
      std::cerr << m_description.value() << std::endl;
    }
    // usage
    std::cerr << std::endl;
    std::cerr << "Usage: " << std::endl;
    std::cerr << "       ";
    std::string name = m_app_name.value_or("PROG");
    std::cerr << name << " ";
    std::cerr << "[OPTIONS]"
              << " ";

    for (const ArgumentValue &arg : m_arguments) {
      std::cerr << "<" + arg.m_name.value_or("ARG") + ">"
                << " ";
    }

    std::cerr << std::endl;
    // options
    std::cerr << std::endl;
    std::cerr << "Options: " << std::endl;
    for (const OptionValue &opt : m_options) {
      std::cerr << "    ";

      if (opt.m_short_name.has_value()) {
        std::cerr << "-" << opt.m_short_name.value() << ", "
                  << "--" << opt.m_name << " <value>";
      } else {
        std::cerr << "    "
                  << "--" << opt.m_name << " <value>";
      }
      if (opt.m_description.has_value()) {
        std::cerr << "     " << opt.m_description.value();
      }
      if (opt.m_default_value.has_value()) {

        std::cerr << " (default: ";
        std::visit([](const auto &value) { std::cerr << value; },
                   opt.m_default_value.value());
        std::cerr << ")";
      }
      std::cerr << std::endl;
    }
    // flag option
    for (const FlagValue &opt : m_flag_options) {
      std::cerr << "    ";
      if (opt.m_short_name.has_value()) {
        std::cerr << "-" << opt.m_short_name.value() << ", "
                  << "--" << opt.m_name;
      } else {
        std::cerr << "    "
                  << "--" << opt.m_name;
      }
      if (opt.m_description.has_value()) {
        std::cerr << "     " << opt.m_description.value();
      }
      std::cerr << std::endl;
    }

    std::cerr << std::endl;
  }
  std::optional<std::string> m_app_name;
  std::optional<std::string> m_description;
  std::vector<ArgumentValue> m_arguments;
  std::vector<OptionValue> m_options;
  std::vector<FlagValue> m_flag_options;

  std::vector<std::string> m_parsed_arguments;
  std::vector<std::pair<std::string, OptionValue>> m_parsed_options;
  std::vector<FlagValue> m_parsed_flag_options;

  std::set<std::string> m_arg_name_set;
  std::set<std::string> m_option_name_set;
  std::set<char> m_option_short_name_set;
};

} // namespace chibi
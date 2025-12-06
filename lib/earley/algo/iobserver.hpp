#pragma once

#include <string>

namespace earley {

struct IParserObserver {
  virtual ~IParserObserver() = default;

  virtual void on_add_item(int /* k */, int /* prod_index */, int /* dot */, int /* start */) = 0;
  virtual void on_parse_begin(const std::string& /* w */) = 0;
  virtual void on_parse_end(const std::string& /* w */, bool /* accepted */) = 0;
};

}  // namespace earley
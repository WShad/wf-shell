#ifndef WIDGETS_LANGUAGE_HPP
#define WIDGETS_LANGUAGE_HPP

#include "../widget.hpp"
#include "gtkmm/button.h"
#include "wf-ipc.hpp"
#include <cstdint>
#include <functional>
#include <gtkmm/calendar.h>
#include <gtkmm/label.h>
#include <iostream>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>
// #include "wayfire-shell-unstable-v2-client-protocol.h"

struct Language
{
  std::string Name;
  std::string ID;
};

class WayfireLanguage : public WayfireWidget
{
    Gtk::Label label;
    Gtk::Button button;

    std::shared_ptr<WayfireIPC> ipc;
    uint32_t current_language;
    std::vector<Language> available;

    std::function<void(nlohmann::json data)> event_handler = [this](nlohmann::json data) {
      if (data["event"] == "keyboard-modifier-state-changed") {
        if (available.size() == 0) {
          set_available(data["state"]["possible-layouts"]);
        }

        auto state_layout = data["state"]["layout-index"];
        if (state_layout != current_language) {
          current_language = state_layout;
          std::cout<<"Layout changed: "<<current_language<<std::endl;
          set_current(state_layout);
        }
      }
    };

  public:
    void init(Gtk::HBox *container) override;
    bool update_label();
    void set_current(uint32_t index);
    void set_available(nlohmann::json languages);
    void next_language();
    WayfireLanguage(std::shared_ptr<WayfireIPC> ipc);
    ~WayfireLanguage();
};

#endif /* end of include guard: WIDGETS_LANGUAGE_HPP */

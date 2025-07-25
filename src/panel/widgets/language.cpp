#include <cstddef>
#include <cstdint>
#include <glibmm.h>
#include <iostream>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>
#include <xkbcommon/xkbregistry.h>
#include "language.hpp"
#include "gtkmm/button.h"
#include "libutil.a.p/wayfire-shell-unstable-v2-client-protocol.h"
#include "sigc++/functors/mem_fun.h"

void WayfireLanguage::init(Gtk::HBox *container)
{
    // button = std::make_unique<Gtk::Button>("panel");

    button.get_style_context()->add_class("language");
    button.add(label);
    button.get_style_context()->add_class("flat");
    button.get_style_context()->remove_class("activated");
    button.signal_clicked().connect_notify(sigc::mem_fun(this, &WayfireLanguage::next_language));
    button.show();
    label.show();

    update_label();

    container->pack_start(button, false, false);
}

bool WayfireLanguage::update_label()
{
    if (current_language >= available.size()) {
        return false;
    }

    label.set_text(available[current_language].ID);
    return true;
}

void WayfireLanguage::set_current(uint32_t index)
{
    current_language = index;
    update_label();
}

void WayfireLanguage::set_available(nlohmann::json languages)
{
    std::vector<Language> languages_available;
    std::map<std::string, uint32_t> names;

    for(size_t i = 0; i < languages.size(); i++) {
        auto elem = languages[i];
        names[elem] = i;
        languages_available.push_back(Language{
            .Name = elem,
            .ID = "",
        });
    }

    auto context = rxkb_context_new(RXKB_CONTEXT_NO_FLAGS);
    rxkb_context_parse_default_ruleset(context);
    auto rlayout = rxkb_layout_first(context);
    for (; rlayout != NULL; rlayout = rxkb_layout_next(rlayout)) {
        auto descr = rxkb_layout_get_description(rlayout);
        auto name = names.find(descr);
        if (name != names.end()) {
            languages_available[name->second].ID = rxkb_layout_get_brief(rlayout);
        }
    }

    available = languages_available;
    update_label();
}

void WayfireLanguage::next_language()
{
    uint32_t next = current_language + 1;
    if (next >= available.size())
    {
        next = 0;
    }

}

WayfireLanguage::WayfireLanguage(std::shared_ptr<WayfireIPC> ipc): ipc(ipc)
{
    ipc->add_handler(event_handler);
    ipc->send_message("{\"method\":\"wayfire/get-keyboard-state\"}", 2000);
    ipc->send_message("{\"method\":\"wayfire/configuration\"}", 2000);
    auto resp = ipc->wait_response();
    std::cout<<"Testing responses: "<<resp<<std::endl;
    return;
    auto r = nlohmann::json::parse(resp);
    set_available(r["possible-layouts"]);
    set_current(r["layout-index"]);
}

WayfireLanguage::~WayfireLanguage()
{}

#include "map_ui.hpp"
#include "gfx/ui.hpp"
#include "../sim/net/net.hpp"
#include "../sim/net/klient.hpp"
#include <log.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>

void Map_UI::update_and_show(const Sub* sub) {
    ui::Font f(ui::FONT::MONO_16);
    sync(objekte.empty());
    handle_imgui_events();
    show_minimap(sub);
}

void Map_UI::sync(bool force) {
    if (static sf::Clock timer; timer.getElapsedTime().asMilliseconds() > SYNC_INTERVALL || force) {
        timer.restart();
        const std::string& objekte_raw = klient->request(Net::ALLE_OBJEKTE);
        zonen = klient->get_zonen();
        teams = klient->get_teams();
        if (!objekte_raw.empty()) {
            try { objekte = Net::deserialize<std::vector<std::unique_ptr<Objekt>>>(objekte_raw); }
            catch (const std::exception& e) {
                Log::err() << "Error: Konnte Antwort auf Net::ALLE_OBJEKTE nicht deserialisieren, Größe: ";
                Log::err() << objekte_raw.size() << " Fehler: " << e.what() << '\n';
            }
        }
    }
}

void Map_UI::handle(sf::Event* event) {
    (void)event;
}

void Map_UI::handle_imgui_events() {
    const auto& io = ImGui::GetIO();
    /// Karte verschieben
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        const auto& delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
        shift_x += 0.5f * delta.x;
        shift_y += 0.5f * delta.y;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
    }
    /// Zoom via Mausrad
    if (!ImGui::IsAnyItemHovered() && std::abs(io.MouseWheel) > 0) {
        scale += (0.25f * scale * io.MouseWheel);
        scale = std::clamp(scale, 0.001f, 1.f);
    }
}

void Map_UI::draw_gfx(const Sub* sub, sf::RenderWindow* window) {
    const auto& pos_sub = sub->get_pos();
    const float size_x = window->getSize().x;
    const float size_y = window->getSize().y;
    const float center_x = 0.5f * size_x;
    const float center_y = 0.5f * size_y;

    const auto world2ui = [&] (float x, float y) {
        return sf::Vector2<float> {
                static_cast<float>(center_x + shift_x + (scale * (x - pos_sub.x()))),
                static_cast<float>(center_y + shift_y - (scale * (y - pos_sub.y())))
        };
    };

    // Ozean zeichnen
    // TODO (=> Terrain)

    // Zonen zeichnen
    for (const Zone& zone : zonen) {
        const auto& pos = world2ui(std::get<0>(zone.get_pos()), std::get<1>(zone.get_pos()));
        const sf::Color color = zone.get_team() == 0 ?             sf::Color(0xFF, 0xFF, 0xFF) : // Kein Besitzer: Weiß
                                zone.get_team() == sub->get_team() ? sf::Color(0x00, 0xFF, 0x00) :   // Eigenes Team: Grün
                                sf::Color(0xFF, 0x00, 0x00);  // Feindlich: Rot
        /*draw_list->AddRect({pos[0] - 0.5f * zone.get_groesse() * scale, pos[1] - 0.5f * zone.get_groesse() * scale},
                           {pos[0] + 0.5f * zone.get_groesse() * scale, pos[1] + 0.5f * zone.get_groesse() * scale},
                           color);*/
        const float r = 0.5f * zone.get_groesse() * scale;
        sf::RectangleShape zone_rect({2 * r, 2 * r});
        zone_rect.setPosition(pos.x - r, pos.y - r);
        zone_rect.setOutlineThickness(2);
        zone_rect.setOutlineColor(color);
        zone_rect.setFillColor(sf::Color::Transparent);
        window->draw(zone_rect);
    }

    // Objekte zeichnen
    for (const auto& o : objekte) {
        //if (fow && o.get_team() != sub->get_team()) continue; // fremdes Team TODO
        //if (x < 0 || x > size_x || y < 0 || y > size_y) continue; // außerhalb des Bildes
        sf::Color color(0xFF, 0xFF, 0xFF);
        if      (o->get_id()   == sub->get_id())        color = sf::Color(0xFF, 0xFF, 0xFF); // eigenes Sub
        else if (o->get_team() != sub->get_team())      color = sf::Color(0xFF, 0, 0); // Feindliches Objekt
        else if (o->get_typ()  == Objekt::Typ::TORPEDO) color = sf::Color(0xFF, 0xFF, 0); // Torpedo
        else if (o->get_typ()  == Objekt::Typ::PING)    color = sf::Color(0, 0, 0xFF); // Sonar Ping
        else if (o->get_team() == sub->get_team())      color = sf::Color(0, 0xFF, 0); // freundliches Objekt
        sf::CircleShape shape(8.f, 4);
        const auto& o_map_pos = world2ui(o->get_pos().x(), o->get_pos().y());
        shape.setPosition(o_map_pos);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(color);
        shape.setOutlineThickness(2);
        window->draw(shape);
    }
}

void Map_UI::show_minimap(const Sub* sub) const {
    (void) sub;
    ImGui::Begin("Map Settings");
    if (ui::Button("Center on Sub")) { shift_x = 0; shift_y = 0; }
    ui::SliderFloat("Scale", &scale, 0.001, 0.5);

    for (const auto& team : teams) {
        ui::Text("Team %u: %u Points", team.first, team.second.get_punkte());
    }
    for (const auto& zone : zonen) {
        ui::Text("Zone @ x=%.0f, y=%.0f, owned by Team %u",
                 std::get<0>(zone.get_pos()), std::get<1>(zone.get_pos()), zone.get_team());
    }
    ImGui::End();
}

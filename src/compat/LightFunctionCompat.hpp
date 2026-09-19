#pragma once

class ActorLightCtrl;
class LightDirector;

namespace smgpc::compat {
LightDirector* active_light_director();
const ActorLightCtrl* registered_player_light_controller();
void unregister_player_light_controller(const ActorLightCtrl*);
}

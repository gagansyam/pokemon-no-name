#include <SDL2/SDL.h>
#include <cmath>
#include <cstdio>
#include "types.hpp"
#include "arena.hpp"
#include "fighter.hpp"
#include "projectile.hpp"
#include "ui.hpp"
#include "ai.hpp"
#include "combat_log.hpp"
#include "camera.hpp"

// Top-level game screen & match state
static GameScreen g_game_screen = SCREEN_START_MENU;
static MatchState g_match_state = MATCH_IN_PROGRESS;
static bool g_show_menu_log = false;
static bool g_show_settings_modal = false;

// Pre-battle menu configuration state
static PokemonType g_menu_p1_type = PKMN_PIKACHU;
static PokemonType g_menu_p2_type = PKMN_CHARIZARD;
static MoveId g_menu_p1_moves[4];
static MoveId g_menu_p2_moves[4];
static MatchType g_menu_match_type = MATCH_TYPE_PLAYER_VS_CPU;
static AIMode g_menu_p1_ai_mode = AI_MODE_BOSS;
static AIMode g_menu_p2_ai_mode = AI_MODE_CASUAL;
static int g_menu_selected_p1_slot = 0;
static int g_menu_selected_p2_slot = 0;
static int g_menu_active_panel = 0; // 0 = Player 1, 1 = CPU Opponent
static float g_menu_anim_timer = 0.0f;
static bool g_spectator_paused = false;
static float g_spectator_speed = 1.0f;
static AimMode g_menu_aim_mode = AIM_MODE_FREE;

// Cinematic K.O. State
static bool g_dynamic_cam = true;
static float g_ko_timer = 0.0f;
static float g_ko_focus_x = 0.0f;
static float g_ko_focus_y = 0.0f;
static MatchState g_post_ko_state = MATCH_VICTORY;

// UI Modal & Drafter States
static int g_drafter_target = 0; // 0 = closed, 1 = P1, 2 = P2
static int g_drafter_slot = 0;   // 0..3 active slot tab

// 1v1 Duel Fighter Instances
static Fighter g_player;
static Fighter g_dummy;
static float g_match_timer = 0.0f;

static const PokemonType PLAYABLE_SPECIES[] = { PKMN_PIKACHU, PKMN_CHARIZARD, PKMN_LUCARIO, PKMN_GRENINJA };
static constexpr int NUM_PLAYABLE_SPECIES = 4;

static PokemonType cycle_species(PokemonType cur, int dir) {
    int cur_idx = 0;
    for (int i = 0; i < NUM_PLAYABLE_SPECIES; ++i) {
        if (PLAYABLE_SPECIES[i] == cur) {
            cur_idx = i;
            break;
        }
    }
    int next_idx = (cur_idx + dir + NUM_PLAYABLE_SPECIES) % NUM_PLAYABLE_SPECIES;
    return PLAYABLE_SPECIES[next_idx];
}

static void cycle_slot_move(int fighter_idx, int slot_idx, int direction) {
    if (slot_idx < 0 || slot_idx >= 4) return;
    Fighter& preview = (fighter_idx == 0) ? g_player : g_dummy;
    MoveId* moves = (fighter_idx == 0) ? g_menu_p1_moves : g_menu_p2_moves;
    if (preview.movepool_count <= 0) return;

    MoveId cur_move = moves[slot_idx];
    int cur_idx = -1;
    for (int k = 0; k < preview.movepool_count; ++k) {
        if (preview.movepool[k] == cur_move) {
            cur_idx = k;
            break;
        }
    }

    // Advance in direction, skipping moves already equipped in other slots
    for (int step = 1; step <= preview.movepool_count; ++step) {
        int next_idx = (cur_idx + direction * step + preview.movepool_count * 10) % preview.movepool_count;
        MoveId candidate = preview.movepool[next_idx];
        bool already_equipped = false;
        for (int s = 0; s < 4; ++s) {
            if (s != slot_idx && moves[s] == candidate) {
                already_equipped = true;
                break;
            }
        }
        if (!already_equipped) {
            moves[slot_idx] = candidate;
            preview.equip_move(slot_idx, candidate);
            break;
        }
    }
}

static void equip_drafter_move(int fighter_idx, int slot_idx, MoveId move_id) {
    if (slot_idx < 0 || slot_idx >= 4) return;
    Fighter& preview = (fighter_idx == 0) ? g_player : g_dummy;
    MoveId* moves = (fighter_idx == 0) ? g_menu_p1_moves : g_menu_p2_moves;

    int existing_slot = -1;
    for (int s = 0; s < 4; ++s) {
        if (moves[s] == move_id) {
            existing_slot = s;
            break;
        }
    }

    if (existing_slot == slot_idx) {
        return; // Already in target slot
    }

    if (existing_slot >= 0) {
        // Atomic swap between slots
        MoveId temp = moves[slot_idx];
        moves[slot_idx] = move_id;
        moves[existing_slot] = temp;
        preview.equip_move(slot_idx, move_id);
        preview.equip_move(existing_slot, temp);
    } else {
        moves[slot_idx] = move_id;
        preview.equip_move(slot_idx, move_id);
    }
}

static void reset_p1_moves() {
    if (g_menu_p1_type == PKMN_PIKACHU) {
        g_menu_p1_moves[0] = MOVE_QUICK_ATTACK;
        g_menu_p1_moves[1] = MOVE_THUNDERBOLT;
        g_menu_p1_moves[2] = MOVE_ELECTRO_BALL;
        g_menu_p1_moves[3] = MOVE_IRON_TAIL;
    } else if (g_menu_p1_type == PKMN_CHARIZARD) {
        g_menu_p1_moves[0] = MOVE_SCRATCH;
        g_menu_p1_moves[1] = MOVE_FLAMETHROWER;
        g_menu_p1_moves[2] = MOVE_FIRE_SPIN;
        g_menu_p1_moves[3] = MOVE_WING_ATTACK;
    } else if (g_menu_p1_type == PKMN_LUCARIO) {
        g_menu_p1_moves[0] = MOVE_FORCE_PALM;
        g_menu_p1_moves[1] = MOVE_AURA_SPHERE;
        g_menu_p1_moves[2] = MOVE_CLOSE_COMBAT;
        g_menu_p1_moves[3] = MOVE_EXTREME_SPEED;
    } else { // PKMN_GRENINJA
        g_menu_p1_moves[0] = MOVE_WATER_SHURIKEN;
        g_menu_p1_moves[1] = MOVE_NIGHT_SLASH;
        g_menu_p1_moves[2] = MOVE_HYDRO_PUMP;
        g_menu_p1_moves[3] = MOVE_AERIAL_ACE;
    }
}

static void reset_p2_moves() {
    if (g_menu_p2_type == PKMN_PIKACHU) {
        g_menu_p2_moves[0] = MOVE_QUICK_ATTACK;
        g_menu_p2_moves[1] = MOVE_THUNDERBOLT;
        g_menu_p2_moves[2] = MOVE_ELECTRO_BALL;
        g_menu_p2_moves[3] = MOVE_IRON_TAIL;
    } else if (g_menu_p2_type == PKMN_CHARIZARD) {
        g_menu_p2_moves[0] = MOVE_SCRATCH;
        g_menu_p2_moves[1] = MOVE_FLAMETHROWER;
        g_menu_p2_moves[2] = MOVE_FIRE_SPIN;
        g_menu_p2_moves[3] = MOVE_WING_ATTACK;
    } else if (g_menu_p2_type == PKMN_LUCARIO) {
        g_menu_p2_moves[0] = MOVE_FORCE_PALM;
        g_menu_p2_moves[1] = MOVE_AURA_SPHERE;
        g_menu_p2_moves[2] = MOVE_CLOSE_COMBAT;
        g_menu_p2_moves[3] = MOVE_EXTREME_SPEED;
    } else { // PKMN_GRENINJA
        g_menu_p2_moves[0] = MOVE_WATER_SHURIKEN;
        g_menu_p2_moves[1] = MOVE_NIGHT_SLASH;
        g_menu_p2_moves[2] = MOVE_HYDRO_PUMP;
        g_menu_p2_moves[3] = MOVE_AERIAL_ACE;
    }
}

static void sync_preview_assets(SDL_Renderer* renderer) {
    g_player.load_assets(renderer, g_menu_p1_type);
    g_dummy.load_assets(renderer, g_menu_p2_type);
    bool p2_is_dummy = (g_menu_match_type == MATCH_TYPE_PLAYER_VS_CPU && g_menu_p2_ai_mode == AI_MODE_DUMMY);
    g_player.init(0, g_menu_p1_type, 1000.0f, 800.0f, false);
    g_dummy.init(1, g_menu_p2_type, 1400.0f, 800.0f, p2_is_dummy);
}

static void start_battle(SDL_Renderer* renderer, int screen_w, int screen_h, bool& locked_on) {
    bool p2_is_dummy = (g_menu_match_type == MATCH_TYPE_PLAYER_VS_CPU && g_menu_p2_ai_mode == AI_MODE_DUMMY);

    g_player.load_assets(renderer, g_menu_p1_type);
    g_player.init(0, g_menu_p1_type, 1000.0f, 800.0f, false);

    g_dummy.load_assets(renderer, g_menu_p2_type);
    g_dummy.init(1, g_menu_p2_type, 1400.0f, 800.0f, p2_is_dummy);

    // Apply configured movesets
    for (int i = 0; i < 4; ++i) {
        g_player.equip_move(i, g_menu_p1_moves[i]);
        g_dummy.equip_move(i, g_menu_p2_moves[i]);
    }

    g_match_timer = 0.0f;
    g_projectiles.init();
    g_arena.clear_decals();
    g_arena.clear_hazards(0);
    g_arena.clear_hazards(1);

    if (g_menu_match_type == MATCH_TYPE_SPECTATOR) {
        g_ai_p1.init(g_menu_p1_ai_mode);
        g_ai_p2.init(g_menu_p2_ai_mode);
    } else {
        if (!p2_is_dummy) g_ai_p2.init(g_menu_p2_ai_mode);
    }

    g_match_state = MATCH_IN_PROGRESS;
    locked_on = (g_menu_aim_mode == AIM_MODE_LOCK_ON);
    g_spectator_paused = false;
    g_ko_timer = 0.0f;
    g_show_menu_log = false;
    g_show_settings_modal = false;
    g_drafter_target = 0;

    CombatLogger::start_match(
        g_menu_match_type,
        g_menu_p1_type, g_menu_p2_type,
        g_menu_p1_moves, g_menu_p2_moves,
        g_menu_p1_ai_mode, g_menu_p2_ai_mode,
        g_menu_aim_mode, g_dynamic_cam
    );

    float init_cam_x, init_cam_y;
    if (g_menu_match_type == MATCH_TYPE_SPECTATOR) {
        float mid_x = (g_player.x + g_dummy.x) * 0.5f;
        float mid_y = (g_player.y + g_dummy.y) * 0.5f;
        init_cam_x = mid_x - screen_w / 2.0f;
        init_cam_y = mid_y - screen_h / 2.0f;
    } else {
        init_cam_x = g_player.x - screen_w / 2.0f;
        init_cam_y = g_player.y - screen_h / 2.0f;
    }
    g_camera.reset(init_cam_x, init_cam_y, 1.0f);
    g_camera.dynamic_enabled = g_dynamic_cam;

    g_game_screen = SCREEN_BATTLE;
}

int main(int /*argc*/, char* /*argv*/[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    int screen_w = 1280;
    int screen_h = 720;

    SDL_Window* window = SDL_CreateWindow(
        "Pokemon Real-Time Battle",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screen_w, screen_h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        printf("CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    bool running = true;
    bool locked_on = false;

    // Initialize default loadouts
    reset_p1_moves();
    reset_p2_moves();
    sync_preview_assets(renderer);

    float mouse_scr_x = (float)(screen_w / 2);
    float mouse_scr_y = (float)(screen_h / 2);

    Uint64 prev_counter = SDL_GetPerformanceCounter();
    float perf_freq = (float)SDL_GetPerformanceFrequency();

    while (running) {
        Uint64 cur_counter = SDL_GetPerformanceCounter();
        float dt = (float)(cur_counter - prev_counter) / perf_freq;
        prev_counter = cur_counter;
        if (dt > 0.05f) dt = 0.05f; // Clamp delta time

        // Cursor visibility: visible in Start Menu, hidden in Arena for crosshair
        if (g_game_screen == SCREEN_START_MENU) {
            SDL_ShowCursor(SDL_ENABLE);
            g_menu_anim_timer += dt;
        } else {
            SDL_ShowCursor(SDL_DISABLE);
        }

        // =====================================================================
        // 1. Process Window and Discrete Events
        // =====================================================================
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            } else if (ev.type == SDL_MOUSEMOTION) {
                mouse_scr_x = (float)ev.motion.x;
                mouse_scr_y = (float)ev.motion.y;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                if (g_game_screen == SCREEN_START_MENU) {
                    if (g_show_menu_log) {
                        g_show_menu_log = false;
                    } else if (g_drafter_target != 0) {
                        int mx = ev.button.x;
                        int my = ev.button.y;
                        int mw_x = 140, mw_y = 36, mw_w = 1000, mw_h = 648;

                        // Close [X] button or Confirm button
                        if ((mx >= mw_x + mw_w - 46 && mx <= mw_x + mw_w - 12 && my >= mw_y + 12 && my <= mw_y + 36) ||
                            (mx >= mw_x + mw_w - 240 && mx <= mw_x + mw_w - 20 && my >= mw_y + 592 && my <= mw_y + 630)) {
                            g_drafter_target = 0;
                        }
                        // Reset button
                        else if (mx >= mw_x + 20 && mx <= mw_x + 240 && my >= mw_y + 592 && my <= mw_y + 630) {
                            if (g_drafter_target == 1) reset_p1_moves();
                            else reset_p2_moves();
                        }
                        // 4 Slot Tabs: y: mw_y + 52 .. mw_y + 104
                        else if (my >= mw_y + 52 && my <= mw_y + 104) {
                            int tab_margin = 20, tab_spacing = 10;
                            int tab_w = (mw_w - tab_margin * 2 - tab_spacing * 3) / 4;
                            for (int i = 0; i < 4; ++i) {
                                int tx = mw_x + tab_margin + i * (tab_w + tab_spacing);
                                if (mx >= tx && mx <= tx + tab_w) {
                                    g_drafter_slot = i;
                                    break;
                                }
                            }
                        }
                        // Movepool Grid: y: mw_y + 122 .. mw_y + 580
                        else if (my >= mw_y + 122 && my <= mw_y + 580) {
                            Fighter& preview = (g_drafter_target == 1) ? g_player : g_dummy;
                            int grid_y = mw_y + 122;
                            int grid_col_w = (mw_w - 40 - 16) / 2;
                            int card_h = 84;
                            int card_gap = 8;
                            for (int k = 0; k < preview.movepool_count; ++k) {
                                int col = k % 2;
                                int row = k / 2;
                                int cx = mw_x + 20 + col * (grid_col_w + 16);
                                int cy = grid_y + row * (card_h + card_gap);
                                if (mx >= cx && mx <= cx + grid_col_w && my >= cy && my <= cy + card_h) {
                                    equip_drafter_move(g_drafter_target - 1, g_drafter_slot, preview.movepool[k]);
                                    break;
                                }
                            }
                        }
                        // Click outside modal card
                        else if (mx < mw_x || mx > mw_x + mw_w || my < mw_y || my > mw_y + mw_h) {
                            g_drafter_target = 0;
                        }
                    } else if (g_show_settings_modal) {
                        int mx = ev.button.x;
                        int my = ev.button.y;

                        // Close button or Apply button or outside modal card
                        if ((mx >= 745 && mx <= 790 && my >= 68 && my <= 100) ||
                            (mx >= 500 && mx <= 760 && my >= 520 && my <= 560) ||
                            (mx < 480 || mx > 800 || my < 60 || my > 640)) {
                            g_show_settings_modal = false;
                        }
                        // 1. Match Mode button: mx: 524..756, my: 116..156
                        else if (mx >= 524 && mx <= 756 && my >= 116 && my <= 156) {
                            g_menu_match_type = (g_menu_match_type == MATCH_TYPE_PLAYER_VS_CPU)
                                ? MATCH_TYPE_SPECTATOR : MATCH_TYPE_PLAYER_VS_CPU;
                        }
                        // 2. AI Mode button: mx: 524..756, my: 162..202
                        else if (mx >= 524 && mx <= 756 && my >= 162 && my <= 202) {
                            if (g_menu_match_type == MATCH_TYPE_PLAYER_VS_CPU) {
                                if (g_menu_p2_ai_mode == AI_MODE_CASUAL) g_menu_p2_ai_mode = AI_MODE_BOSS;
                                else if (g_menu_p2_ai_mode == AI_MODE_BOSS) g_menu_p2_ai_mode = AI_MODE_DUMMY;
                                else g_menu_p2_ai_mode = AI_MODE_CASUAL;
                            } else {
                                if (g_menu_p1_ai_mode == AI_MODE_BOSS && g_menu_p2_ai_mode == AI_MODE_BOSS) {
                                    g_menu_p1_ai_mode = AI_MODE_CASUAL;
                                    g_menu_p2_ai_mode = AI_MODE_CASUAL;
                                } else if (g_menu_p1_ai_mode == AI_MODE_CASUAL && g_menu_p2_ai_mode == AI_MODE_CASUAL) {
                                    g_menu_p1_ai_mode = AI_MODE_CASUAL;
                                    g_menu_p2_ai_mode = AI_MODE_BOSS;
                                } else if (g_menu_p1_ai_mode == AI_MODE_CASUAL && g_menu_p2_ai_mode == AI_MODE_BOSS) {
                                    g_menu_p1_ai_mode = AI_MODE_BOSS;
                                    g_menu_p2_ai_mode = AI_MODE_CASUAL;
                                } else {
                                    g_menu_p1_ai_mode = AI_MODE_BOSS;
                                    g_menu_p2_ai_mode = AI_MODE_BOSS;
                                }
                            }
                        }
                        // 3. Aim Mode button: mx: 524..756, my: 208..248
                        else if (mx >= 524 && mx <= 756 && my >= 208 && my <= 248) {
                            g_menu_aim_mode = (g_menu_aim_mode == AIM_MODE_FREE) ? AIM_MODE_LOCK_ON : AIM_MODE_FREE;
                        }
                        // 4. Camera Mode button: mx: 524..756, my: 254..294
                        else if (mx >= 524 && mx <= 756 && my >= 254 && my <= 294) {
                            g_dynamic_cam = !g_dynamic_cam;
                        }
                    } else if (ev.button.button == SDL_BUTTON_LEFT) {
                        int mx = ev.button.x;
                        int my = ev.button.y;

                        // Top Header Settings Button: screen_w - 240 .. screen_w - 110, my: 16..48
                        // Or Center Card Settings Button: mx: 524..756, my: 374..418
                        if ((mx >= screen_w - 240 && mx <= screen_w - 110 && my >= 16 && my <= 48) ||
                            (mx >= 524 && mx <= 756 && my >= 374 && my <= 418)) {
                            g_show_settings_modal = true;
                        }
                        // Top Header Log Button: screen_w - 100 .. screen_w - 15, my: 16..48
                        // Or Center Card Log Button: mx: 524..756, my: 430..474
                        else if (((mx >= screen_w - 100 && mx <= screen_w - 15 && my >= 16 && my <= 48) ||
                                  (mx >= 524 && mx <= 756 && my >= 430 && my <= 474)) && CombatLogger::has_completed_log()) {
                            g_show_menu_log = true;
                        }
                        // P1 Switcher: mx: 42..515, my: 98..130
                        else if (my >= 98 && my <= 130 && mx >= 42 && mx <= 515) {
                            if (mx < 42 + (515 - 42) / 2) {
                                g_menu_p1_type = cycle_species(g_menu_p1_type, -1);
                            } else {
                                g_menu_p1_type = cycle_species(g_menu_p1_type, 1);
                            }
                            reset_p1_moves();
                            g_menu_selected_p1_slot = 0;
                            sync_preview_assets(renderer);
                        }
                        // P2 Switcher: mx: 775..1245, my: 98..130
                        else if (my >= 98 && my <= 130 && mx >= 775 && mx <= 1245) {
                            if (mx < 775 + (1245 - 775) / 2) {
                                g_menu_p2_type = cycle_species(g_menu_p2_type, -1);
                            } else {
                                g_menu_p2_type = cycle_species(g_menu_p2_type, 1);
                            }
                            reset_p2_moves();
                            g_menu_selected_p2_slot = 0;
                            sync_preview_assets(renderer);
                        }
                        // Start Battle Button (Test 69 backward compatibility): mx: 524..756, my: 302..358
                        else if (mx >= 524 && mx <= 756 && my >= 302 && my <= 358) {
                            start_battle(renderer, screen_w, screen_h, locked_on);
                        }
                        // Match Type button (Test 69 backward compatibility): mx: 524..756, my: 116..156
                        else if (mx >= 524 && mx <= 756 && my >= 116 && my <= 156) {
                            g_menu_match_type = (g_menu_match_type == MATCH_TYPE_PLAYER_VS_CPU)
                                ? MATCH_TYPE_SPECTATOR : MATCH_TYPE_PLAYER_VS_CPU;
                        }
                        // AI Mode button (Test 69 backward compatibility): mx: 524..756, my: 162..202
                        else if (mx >= 524 && mx <= 756 && my >= 162 && my <= 202) {
                            if (g_menu_match_type == MATCH_TYPE_PLAYER_VS_CPU) {
                                if (g_menu_p2_ai_mode == AI_MODE_CASUAL) g_menu_p2_ai_mode = AI_MODE_BOSS;
                                else if (g_menu_p2_ai_mode == AI_MODE_BOSS) g_menu_p2_ai_mode = AI_MODE_DUMMY;
                                else g_menu_p2_ai_mode = AI_MODE_CASUAL;
                            }
                        }
                        // Aim Mode button (Test 69 backward compatibility): mx: 524..756, my: 208..248
                        else if (mx >= 524 && mx <= 756 && my >= 208 && my <= 248) {
                            g_menu_aim_mode = (g_menu_aim_mode == AIM_MODE_FREE) ? AIM_MODE_LOCK_ON : AIM_MODE_FREE;
                        }
                        // Camera Mode button (Test 69 backward compatibility): mx: 524..756, my: 254..294
                        else if (mx >= 524 && mx <= 756 && my >= 254 && my <= 294) {
                            g_dynamic_cam = !g_dynamic_cam;
                        }
                        // P1 Slot Interaction: y: 416..560, x: 59..491
                        else if (mx >= 59 && mx <= 491 && my >= 416 && my <= 560) {
                            int slot = (my - 416) / 36;
                            if (slot >= 0 && slot < 4) {
                                int sy = 416 + slot * 36;
                                if (mx >= 59 + 4 && mx <= 59 + 24 && my >= sy + 4 && my <= sy + 28) {
                                    cycle_slot_move(0, slot, -1);
                                } else if (mx >= 491 - 24 && mx <= 491 - 4 && my >= sy + 4 && my <= sy + 28) {
                                    cycle_slot_move(0, slot, +1);
                                } else {
                                    g_drafter_target = 1;
                                    g_drafter_slot = slot;
                                }
                            }
                        }
                        // P1 Draft Full Moveset Button: mx: 59..491, my: 568..606
                        else if (mx >= 59 && mx <= 491 && my >= 568 && my <= 606) {
                            g_drafter_target = 1;
                            g_drafter_slot = 0;
                        }
                        // P2 Slot Interaction: y: 416..560, x: 789..1221
                        else if (mx >= 789 && mx <= 1221 && my >= 416 && my <= 560) {
                            int slot = (my - 416) / 36;
                            if (slot >= 0 && slot < 4) {
                                int sy = 416 + slot * 36;
                                if (mx >= 789 + 4 && mx <= 789 + 24 && my >= sy + 4 && my <= sy + 28) {
                                    cycle_slot_move(1, slot, -1);
                                } else if (mx >= 1221 - 24 && mx <= 1221 - 4 && my >= sy + 4 && my <= sy + 28) {
                                    cycle_slot_move(1, slot, +1);
                                } else {
                                    g_drafter_target = 2;
                                    g_drafter_slot = slot;
                                }
                            }
                        }
                        // P2 Draft Full Moveset Button: mx: 789..1221, my: 568..606
                        else if (mx >= 789 && mx <= 1221 && my >= 568 && my <= 606) {
                            g_drafter_target = 2;
                            g_drafter_slot = 0;
                        }
                    }
                } else if (g_game_screen == SCREEN_BATTLE) {
                    if (ev.button.button == SDL_BUTTON_MIDDLE) {
                        locked_on = !locked_on;
                    } else if (g_menu_match_type != MATCH_TYPE_SPECTATOR && g_match_state == MATCH_IN_PROGRESS) {
                        float target_wx = (locked_on && g_dummy.hp > 0) ? g_dummy.x : (g_camera.render_x + mouse_scr_x / g_camera.zoom);
                        float target_wy = (locked_on && g_dummy.hp > 0) ? (g_dummy.y - g_dummy.z) : (g_camera.render_y + mouse_scr_y / g_camera.zoom);

                        if (ev.button.button == SDL_BUTTON_LEFT) {
                            g_player.use_ability(0, &g_dummy, target_wx, target_wy); // Slot 0: Primary Attack
                        } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                            g_player.use_ability(1, &g_dummy, target_wx, target_wy); // Slot 1: Secondary Move
                        }
                    }
                }
            } else if (ev.type == SDL_KEYDOWN) {
                if (g_game_screen == SCREEN_START_MENU) {
                    if (g_drafter_target != 0) {
                        if (ev.key.keysym.sym == SDLK_ESCAPE || ev.key.keysym.sym == SDLK_RETURN ||
                            ev.key.keysym.sym == SDLK_KP_ENTER || ev.key.keysym.sym == SDLK_SPACE) {
                            g_drafter_target = 0;
                        } else if (ev.key.keysym.sym == SDLK_1) {
                            g_drafter_slot = 0;
                        } else if (ev.key.keysym.sym == SDLK_2) {
                            g_drafter_slot = 1;
                        } else if (ev.key.keysym.sym == SDLK_3) {
                            g_drafter_slot = 2;
                        } else if (ev.key.keysym.sym == SDLK_4) {
                            g_drafter_slot = 3;
                        } else if (ev.key.keysym.sym == SDLK_m) {
                            if (g_drafter_target == 1) g_drafter_target = 0;
                            else { g_drafter_target = 1; g_drafter_slot = 0; }
                        } else if (ev.key.keysym.sym == SDLK_n) {
                            if (g_drafter_target == 2) g_drafter_target = 0;
                            else { g_drafter_target = 2; g_drafter_slot = 0; }
                        }
                    } else if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_KP_ENTER || ev.key.keysym.sym == SDLK_SPACE) {
                        if (g_show_menu_log) {
                            g_show_menu_log = false;
                        } else if (g_show_settings_modal) {
                            g_show_settings_modal = false;
                        } else {
                            start_battle(renderer, screen_w, screen_h, locked_on);
                        }
                    } else if (ev.key.keysym.sym == SDLK_m) {
                        g_drafter_target = 1;
                        g_drafter_slot = 0;
                    } else if (ev.key.keysym.sym == SDLK_n) {
                        g_drafter_target = 2;
                        g_drafter_slot = 0;
                    } else if (ev.key.keysym.sym == SDLK_s) {
                        g_show_settings_modal = !g_show_settings_modal;
                    } else if (ev.key.keysym.sym == SDLK_l && ev.key.repeat == 0 && CombatLogger::has_completed_log()) {
                        g_show_menu_log = !g_show_menu_log;
                    } else if (ev.key.keysym.sym == SDLK_ESCAPE) {
                        if (g_show_menu_log) {
                            g_show_menu_log = false;
                        } else if (g_show_settings_modal) {
                            g_show_settings_modal = false;
                        } else {
                            running = false;
                        }
                    } else if (ev.key.keysym.sym == SDLK_c) {
                        g_menu_p1_type = cycle_species(g_menu_p1_type, 1);
                        reset_p1_moves();
                        g_menu_selected_p1_slot = 0;
                        sync_preview_assets(renderer);
                    } else if (ev.key.keysym.sym == SDLK_v) {
                        g_menu_p2_type = cycle_species(g_menu_p2_type, 1);
                        reset_p2_moves();
                        g_menu_selected_p2_slot = 0;
                        sync_preview_assets(renderer);
                    } else if (ev.key.keysym.sym == SDLK_t) {
                        g_menu_match_type = (g_menu_match_type == MATCH_TYPE_PLAYER_VS_CPU)
                            ? MATCH_TYPE_SPECTATOR : MATCH_TYPE_PLAYER_VS_CPU;
                    } else if (ev.key.keysym.sym == SDLK_y) {
                        if (g_menu_match_type == MATCH_TYPE_PLAYER_VS_CPU) {
                            if (g_menu_p2_ai_mode == AI_MODE_CASUAL) g_menu_p2_ai_mode = AI_MODE_BOSS;
                            else if (g_menu_p2_ai_mode == AI_MODE_BOSS) g_menu_p2_ai_mode = AI_MODE_DUMMY;
                            else g_menu_p2_ai_mode = AI_MODE_CASUAL;
                        } else {
                            if (g_menu_p1_ai_mode == AI_MODE_BOSS && g_menu_p2_ai_mode == AI_MODE_BOSS) {
                                g_menu_p1_ai_mode = AI_MODE_CASUAL;
                                g_menu_p2_ai_mode = AI_MODE_CASUAL;
                            } else if (g_menu_p1_ai_mode == AI_MODE_CASUAL && g_menu_p2_ai_mode == AI_MODE_CASUAL) {
                                g_menu_p1_ai_mode = AI_MODE_CASUAL;
                                g_menu_p2_ai_mode = AI_MODE_BOSS;
                            } else if (g_menu_p1_ai_mode == AI_MODE_CASUAL && g_menu_p2_ai_mode == AI_MODE_BOSS) {
                                g_menu_p1_ai_mode = AI_MODE_BOSS;
                                g_menu_p2_ai_mode = AI_MODE_CASUAL;
                            } else {
                                g_menu_p1_ai_mode = AI_MODE_BOSS;
                                g_menu_p2_ai_mode = AI_MODE_BOSS;
                            }
                        }
                    } else if (ev.key.keysym.sym == SDLK_z) {
                        g_menu_aim_mode = (g_menu_aim_mode == AIM_MODE_FREE) ? AIM_MODE_LOCK_ON : AIM_MODE_FREE;
                    } else if (ev.key.keysym.sym == SDLK_g) {
                        g_dynamic_cam = !g_dynamic_cam;
                    }
                } else if (g_game_screen == SCREEN_BATTLE) {
                    if (ev.key.keysym.sym == SDLK_ESCAPE) {
                        g_game_screen = SCREEN_START_MENU;
                        sync_preview_assets(renderer);
                    } else if (ev.key.keysym.sym == SDLK_l) {
                        if (g_match_state == MATCH_VICTORY || g_match_state == MATCH_DEFEAT) {
                            g_game_screen = SCREEN_START_MENU;
                            sync_preview_assets(renderer);
                        }
                    } else if (ev.key.keysym.sym == SDLK_r) {
                        start_battle(renderer, screen_w, screen_h, locked_on);
                    } else if (g_menu_match_type == MATCH_TYPE_SPECTATOR) {
                        if (ev.key.keysym.sym == SDLK_SPACE) {
                            g_spectator_paused = !g_spectator_paused;
                        } else if (ev.key.keysym.sym == SDLK_1) {
                            g_spectator_speed = 1.0f;
                        } else if (ev.key.keysym.sym == SDLK_2) {
                            g_spectator_speed = 1.5f;
                        } else if (ev.key.keysym.sym == SDLK_3) {
                            g_spectator_speed = 2.0f;
                        } else if (ev.key.keysym.sym == SDLK_0) {
                            g_spectator_speed = 0.5f;
                        }
                    } else {
                        if (g_match_state == MATCH_IN_PROGRESS) {
                            float target_wx = (locked_on && g_dummy.hp > 0) ? g_dummy.x : (g_camera.render_x + mouse_scr_x / g_camera.zoom);
                            float target_wy = (locked_on && g_dummy.hp > 0) ? (g_dummy.y - g_dummy.z) : (g_camera.render_y + mouse_scr_y / g_camera.zoom);

                            if (ev.key.keysym.sym == SDLK_TAB) {
                                locked_on = !locked_on;
                            } else if (ev.key.keysym.sym == SDLK_SPACE) {
                                float ddx = 0.0f, ddy = 0.0f;
                                const Uint8* ks = SDL_GetKeyboardState(NULL);
                                if (ks[SDL_SCANCODE_W] || ks[SDL_SCANCODE_UP])    ddy -= 1.0f;
                                if (ks[SDL_SCANCODE_S] || ks[SDL_SCANCODE_DOWN])  ddy += 1.0f;
                                if (ks[SDL_SCANCODE_A] || ks[SDL_SCANCODE_LEFT])  ddx -= 1.0f;
                                if (ks[SDL_SCANCODE_D] || ks[SDL_SCANCODE_RIGHT]) ddx += 1.0f;
                                g_player.try_dash(ddx, ddy);
                            } else if (ev.key.keysym.sym == SDLK_1) {
                                g_player.use_ability(0, &g_dummy, target_wx, target_wy); // Slot 0
                            } else if (ev.key.keysym.sym == SDLK_2) {
                                g_player.use_ability(1, &g_dummy, target_wx, target_wy); // Slot 1
                            } else if (ev.key.keysym.sym == SDLK_3 || ev.key.keysym.sym == SDLK_LSHIFT) {
                                g_player.use_ability(2, &g_dummy, target_wx, target_wy); // Slot 2
                            } else if (ev.key.keysym.sym == SDLK_4 || ev.key.keysym.sym == SDLK_f) {
                                g_player.use_ability(3, &g_dummy, target_wx, target_wy); // Slot 3
                            } else if (ev.key.keysym.sym == SDLK_q) {
                                g_player.use_ability(2, &g_dummy, target_wx, target_wy); // Fallback Slot 2
                            } else if (ev.key.keysym.sym == SDLK_e) {
                                g_player.use_ability(3, &g_dummy, target_wx, target_wy); // Fallback Slot 3
                            }
                        }
                    }
                }
            }
        }

        // =====================================================================
        // 2. State-Specific Updates & Physics
        // =====================================================================
        if (g_game_screen == SCREEN_START_MENU) {
            g_ui.render_start_menu(
                renderer,
                g_player, g_dummy,
                g_menu_p1_type, g_menu_p2_type,
                g_menu_p1_moves, g_menu_p2_moves,
                g_menu_selected_p1_slot, g_menu_selected_p2_slot, g_menu_active_panel,
                g_menu_match_type, g_menu_p1_ai_mode, g_menu_p2_ai_mode,
                g_menu_aim_mode, g_dynamic_cam,
                mouse_scr_x, mouse_scr_y,
                screen_w, screen_h,
                g_menu_anim_timer,
                g_show_menu_log,
                g_show_settings_modal,
                g_drafter_target,
                g_drafter_slot
            );
            SDL_RenderPresent(renderer);
            continue;
        }

        // --- Active Battle Arena Loop ---
        float sim_dt = dt;
        if (g_match_state == MATCH_CINEMATIC_KO) {
            sim_dt = dt * 0.15f; // Cinematic slow-motion on final blow
        } else if (g_menu_match_type == MATCH_TYPE_SPECTATOR) {
            sim_dt = g_spectator_paused ? 0.0f : (dt * g_spectator_speed);
        } else {
            if (g_match_state == MATCH_IN_PROGRESS) {
                const Uint8* kstate = SDL_GetKeyboardState(NULL);
                float in_x = 0.0f, in_y = 0.0f;
                if (kstate[SDL_SCANCODE_W] || kstate[SDL_SCANCODE_UP])    in_y -= 1.0f;
                if (kstate[SDL_SCANCODE_S] || kstate[SDL_SCANCODE_DOWN])  in_y += 1.0f;
                if (kstate[SDL_SCANCODE_A] || kstate[SDL_SCANCODE_LEFT])  in_x -= 1.0f;
                if (kstate[SDL_SCANCODE_D] || kstate[SDL_SCANCODE_RIGHT]) in_x += 1.0f;
                g_player.input_dx = in_x;
                g_player.input_dy = in_y;
            } else {
                g_player.input_dx = 0.0f;
                g_player.input_dy = 0.0f;
            }

            // Continuous Aiming
            int cur_mx, cur_my;
            SDL_GetMouseState(&cur_mx, &cur_my);
            mouse_scr_x = (float)cur_mx;
            mouse_scr_y = (float)cur_my;

            float mouse_world_x = g_camera.render_x + mouse_scr_x / g_camera.zoom;
            float mouse_world_y = g_camera.render_y + mouse_scr_y / g_camera.zoom;

            if (locked_on && g_dummy.hp <= 0) {
                locked_on = false;
            }

            if (locked_on) {
                g_player.aim_angle = atan2f((g_dummy.y - g_dummy.z) - g_player.y, g_dummy.x - g_player.x);
            } else {
                g_player.aim_angle = atan2f(mouse_world_y - g_player.y, mouse_world_x - g_player.x);
            }
        }

        // Game State & Physics Update (with Global Hitstop support)
        if (g_projectiles.hitstop_timer > 0.0f) {
            g_projectiles.hitstop_timer -= sim_dt;
            if (g_projectiles.hitstop_timer < 0.0f) g_projectiles.hitstop_timer = 0.0f;
            g_projectiles.update_effects(sim_dt);
        } else if (sim_dt > 0.0f) {
            CombatLogger::update(sim_dt);
            if (g_match_state == MATCH_IN_PROGRESS) {
                g_match_timer += sim_dt;
            }
            if (g_menu_match_type == MATCH_TYPE_SPECTATOR) {
                if (g_match_state == MATCH_IN_PROGRESS || g_match_state == MATCH_CINEMATIC_KO) {
                    g_ai_p1.update(&g_player, &g_dummy, sim_dt);
                    g_ai_p2.update(&g_dummy, &g_player, sim_dt);
                }
            } else {
                bool p2_is_dummy = (g_menu_p2_ai_mode == AI_MODE_DUMMY);
                if (!p2_is_dummy && (g_match_state == MATCH_IN_PROGRESS || g_match_state == MATCH_CINEMATIC_KO)) {
                    g_ai_p2.update(&g_dummy, &g_player, sim_dt);
                } else if (p2_is_dummy) {
                    g_dummy.input_dx = 0.0f;
                    g_dummy.input_dy = 0.0f;
                    g_dummy.aim_angle = atan2f(g_player.y - g_dummy.y, g_player.x - g_dummy.x);
                }
            }

            g_player.update(sim_dt, &g_dummy);
            g_dummy.update(sim_dt, &g_player);
            g_projectiles.update(sim_dt, g_player.x, g_player.y, g_dummy.x, g_dummy.y);
            g_arena.update(sim_dt);

            // Melee Attack Clash (Anime Cross-Counter)
            g_projectiles.check_melee_clash(&g_player, &g_dummy);

            // Collision & Damage checks (bidirectional)
            g_player.check_hits(&g_dummy);
            g_dummy.check_hits(&g_player);
        }

        // Match state transitions upon faint
        if (g_match_state == MATCH_IN_PROGRESS) {
            if (g_dummy.hp <= 0 && !g_dummy.is_dummy) {
                CombatLogger::end_match(0);
                g_match_state = MATCH_CINEMATIC_KO;
                g_ko_timer = 0.70f;
                g_ko_focus_x = g_dummy.x;
                g_ko_focus_y = g_dummy.y;
                g_post_ko_state = MATCH_VICTORY;
                g_projectiles.screen_shake = 0.35f;
                locked_on = false;
            } else if (g_player.hp <= 0) {
                CombatLogger::end_match(1);
                g_match_state = MATCH_CINEMATIC_KO;
                g_ko_timer = 0.70f;
                g_ko_focus_x = g_player.x;
                g_ko_focus_y = g_player.y;
                g_post_ko_state = MATCH_DEFEAT;
                g_projectiles.screen_shake = 0.35f;
                locked_on = false;
            }
        } else if (g_match_state == MATCH_CINEMATIC_KO) {
            g_ko_timer -= dt;
            if (g_ko_timer <= 0.0f) {
                g_match_state = g_post_ko_state;
            }
        }

        // Camera Tracking & Viewport Zoom Update
        g_camera.dynamic_enabled = g_dynamic_cam;
        g_camera.update(
            dt, g_player, g_dummy,
            mouse_scr_x, mouse_scr_y,
            screen_w, screen_h,
            (int)g_match_state, g_ko_focus_x, g_ko_focus_y,
            (g_menu_match_type == MATCH_TYPE_SPECTATOR || (locked_on && g_dummy.hp > 0)),
            g_projectiles.hitstop_timer, g_projectiles.screen_shake
        );

        // 1. World Space Rendering at Dynamic Zoom
        SDL_RenderSetScale(renderer, g_camera.zoom, g_camera.zoom);
        g_arena.render(renderer, g_camera.render_x, g_camera.render_y, (int)g_camera.view_w, (int)g_camera.view_h);
        g_dummy.render(renderer, g_camera.render_x, g_camera.render_y);
        g_player.render(renderer, g_camera.render_x, g_camera.render_y);
        g_projectiles.render(renderer, g_camera.render_x, g_camera.render_y);

        // 2. Screen Space HUD & Cinematic Overlays at 1:1 Scale
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);

        bool is_ai_active = (g_menu_match_type == MATCH_TYPE_SPECTATOR) || (g_menu_p2_ai_mode != AI_MODE_DUMMY);

        if (g_match_state == MATCH_CINEMATIC_KO) {
            g_ui.render_cinematic_ko(renderer, g_ko_focus_x, g_ko_focus_y,
                                     g_camera.render_x, g_camera.render_y, g_camera.zoom,
                                     g_ko_timer, screen_w, screen_h);
        } else {
            g_ui.render_battle_hud(renderer, g_player, g_dummy,
                                   mouse_scr_x, mouse_scr_y,
                                   g_camera.render_x, g_camera.render_y,
                                   screen_w, screen_h, locked_on,
                                   is_ai_active, (int)g_match_state,
                                   g_menu_match_type,
                                   g_menu_p1_ai_mode, g_menu_p2_ai_mode,
                                   g_spectator_paused, g_spectator_speed,
                                   g_camera.zoom, g_dynamic_cam,
                                   g_menu_aim_mode,
                                   &g_player, 0, 1,
                                   &g_dummy, 0, 1,
                                   g_match_timer);
        }

        if (g_match_state == MATCH_VICTORY || g_match_state == MATCH_DEFEAT) {
            g_ui.render_post_match_analytics(renderer, CombatLogger::get_last_completed_log(), screen_w, screen_h, false);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

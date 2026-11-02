#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>
#include <cstring>
#include "types.hpp"
#include "arena.hpp"
#include "fighter.hpp"
#include "projectile.hpp"
#include "moves.hpp"
#include "ai.hpp"

// Headless test suite for Pokémon Battlegrounds: 1v1 Real-Time Duel Simulation
int main() {
    printf("=================================================================\n");
    printf("   RUNNING AUTOMATED TEST SUITE: POKÉMON BATTLEGROUNDS 1v1 SIM   \n");
    printf("=================================================================\n\n");

    // -----------------------------------------------------------------
    // TEST 1: Canonical Base Stats & Dual Typing for 3 Playable Species
    // -----------------------------------------------------------------
    printf("[TEST 1] Verifying 3 Playable Species Canonical Typing & Base Stats...\n");
    ElementType t1, t2;
    TypeChart::get_species_types(PKMN_PIKACHU, t1, t2);
    assert(t1 == TYPE_ELECTRIC && t2 == TYPE_NONE);
    assert(TypeChart::get_base_stats(PKMN_PIKACHU).speed == 110);
    assert(TypeChart::get_base_stats(PKMN_PIKACHU).hp == 540);

    TypeChart::get_species_types(PKMN_CHARIZARD, t1, t2);
    assert(t1 == TYPE_FIRE && t2 == TYPE_FLYING);
    assert(TypeChart::get_base_stats(PKMN_CHARIZARD).attack == 105);
    assert(TypeChart::get_base_stats(PKMN_CHARIZARD).hp == 660);

    TypeChart::get_species_types(PKMN_LUCARIO, t1, t2);
    assert(t1 == TYPE_FIGHTING && t2 == TYPE_STEEL);
    assert(TypeChart::get_base_stats(PKMN_LUCARIO).attack == 110);
    assert(TypeChart::get_base_stats(PKMN_LUCARIO).hp == 600);
    printf("  -> PASS: All 3 playable species definitions, dual types, and canonical stats verified.\n\n");

    // -----------------------------------------------------------------
    // TEST 2: Stat Stage Multipliers & Real-Time Damage Formula
    // -----------------------------------------------------------------
    printf("[TEST 2] Verifying Stat Stages, Burn Penalty & Blaze Multiplier...\n");
    assert(fabsf(TypeChart::get_stage_multiplier(0) - 1.0f) < 0.001f);
    assert(fabsf(TypeChart::get_stage_multiplier(+2) - 2.0f) < 0.001f);
    assert(fabsf(TypeChart::get_stage_multiplier(-2) - 0.5f) < 0.001f);
    assert(fabsf(TypeChart::get_stage_multiplier(+6) - 4.0f) < 0.001f);
    assert(fabsf(TypeChart::get_stage_multiplier(-6) - 0.25f) < 0.001f);

    // Damage with and without Attack Stat Stage boost
    PokemonStats charizard_stats = TypeChart::get_base_stats(PKMN_CHARIZARD);
    PokemonStats lucario_stats = TypeChart::get_base_stats(PKMN_LUCARIO);
    float mult = 1.0f;

    // Lucario (Fighting/Steel) hits Charizard (Fire/Flying) with Physical Fighting move (0.5x resisted)
    int dmg_neutral = TypeChart::calculate_damage(
        80, CATEGORY_PHYSICAL, TYPE_FIGHTING,
        lucario_stats, TYPE_FIGHTING, TYPE_STEEL,
        charizard_stats, TYPE_FIRE, TYPE_FLYING,
        0, 0, false, false, &mult
    );
    assert(mult < 1.0f); // Resisted by Fire/Flying!

    int dmg_boosted = TypeChart::calculate_damage(
        80, CATEGORY_PHYSICAL, TYPE_FIGHTING,
        lucario_stats, TYPE_FIGHTING, TYPE_STEEL,
        charizard_stats, TYPE_FIRE, TYPE_FLYING,
        2, 0, false, false, &mult
    );
    assert(dmg_boosted > dmg_neutral);

    // Burn penalty on physical move
    int dmg_burned = TypeChart::calculate_damage(
        80, CATEGORY_PHYSICAL, TYPE_FIGHTING,
        lucario_stats, TYPE_FIGHTING, TYPE_STEEL,
        charizard_stats, TYPE_FIRE, TYPE_FLYING,
        0, 0, true, false, &mult
    );
    assert(dmg_burned < dmg_neutral);

    // Blaze mode boost on Fire move (Flamethrower) for Charizard
    int dmg_fire_normal = TypeChart::calculate_damage(
        90, CATEGORY_SPECIAL, TYPE_FIRE,
        charizard_stats, TYPE_FIRE, TYPE_FLYING,
        lucario_stats, TYPE_FIGHTING, TYPE_STEEL,
        0, 0, false, false, &mult
    );
    int dmg_fire_blaze = TypeChart::calculate_damage(
        90, CATEGORY_SPECIAL, TYPE_FIRE,
        charizard_stats, TYPE_FIRE, TYPE_FLYING,
        lucario_stats, TYPE_FIGHTING, TYPE_STEEL,
        0, 0, false, true, &mult
    );
    assert(dmg_fire_blaze > (int)((float)dmg_fire_normal * 1.5f));
    printf("  -> PASS: Dynamic stat stages, burn physical penalty, and Blaze 1.6x multiplier verified.\n\n");

    // -----------------------------------------------------------------
    // TEST 3: 1v1 Duel Setup & Canonical Fighter Initialization
    // -----------------------------------------------------------------
    printf("[TEST 3] Verifying 1v1 Duel Match Setup & Fighter State...\n");
    Fighter p1_duel;
    Fighter p2_duel;

    p1_duel.init(0, PKMN_PIKACHU, 1000.0f, 800.0f, false);
    p2_duel.init(1, PKMN_CHARIZARD, 1400.0f, 800.0f, false);

    assert(p1_duel.hp == p1_duel.max_hp);
    assert(p2_duel.hp == p2_duel.max_hp);
    assert(!p1_duel.is_bench);
    assert(!p2_duel.is_bench);
    assert(p1_duel.type == PKMN_PIKACHU);
    assert(p2_duel.type == PKMN_CHARIZARD);

    p1_duel.modify_stat_stage(0, 2); // +2 Atk
    p1_duel.modify_stat_stage(1, -1); // -1 Def
    assert(p1_duel.stage_atk == 2);
    assert(p1_duel.stage_def == -1);
    printf("  -> PASS: 1v1 duel participants initialized with full HP, zero bench overhead, and valid stat modifiers.\n\n");

    // -----------------------------------------------------------------
    // TEST 4: Charizard Wing Attack Complete Lifecycle & Unfreeze Verification
    // -----------------------------------------------------------------
    printf("[TEST 4] Verifying Charizard Wing Attack Lifecycle & Unfreeze...\n");
    Fighter charizard;
    charizard.init(1, PKMN_CHARIZARD, 1200.0f, 800.0f, false);
    charizard.aim_angle = 3.14159f; // Facing West towards player

    // Execute Wing Attack
    MoveRegistry::execute(MOVE_WING_ATTACK, &charizard, &p1_duel, p1_duel.x, p1_duel.y);
    assert(charizard.state == STATE_ATTACK);
    assert(charizard.active_move == MOVE_WING_ATTACK);
    assert(charizard.is_flying == true);
    assert(charizard.attack_timer > 0.60f);

    // Simulate 0.75 seconds (45 frames at 60 FPS) to advance past full 0.68s duration
    for (int frame = 0; frame < 50; ++frame) {
        charizard.update(0.0166f, &p1_duel);
    }

    // Verify move is completely terminated and flight state cleanly cleared
    assert(charizard.attack_timer <= 0.0f);
    assert(charizard.state == STATE_IDLE);
    assert(charizard.active_move == MOVE_NONE);
    assert(charizard.is_flying == false);
    assert(charizard.wing_attack_rush == 0.0f);
    assert(charizard.wing_attack_windup == 0.0f);
    assert(charizard.z == 0.0f);

    // Verify AI successfully provides input and moves without freeze
    BattleAI cpu_ai;
    cpu_ai.init(AI_MODE_BOSS);
    cpu_ai.update(&charizard, &p1_duel, 0.0166f);
    assert(!charizard.is_flying);
    // Charizard should now be pursuing the player with non-zero movement input
    assert(charizard.input_dx != 0.0f || charizard.input_dy != 0.0f);
    printf("  -> PASS: Charizard Wing Attack executes full 2-phase trajectory, exits flight, and AI resumes active movement.\n\n");

    // -----------------------------------------------------------------
    // TEST 5: Toxic Spikes Hazard Creation, Collision & Absorption
    // -----------------------------------------------------------------
    printf("[TEST 5] Verifying Arena Toxic Spikes & Poison Absorption...\n");
    g_arena.clear_hazards(0);
    g_arena.clear_hazards(1);

    // Paul's side sets Toxic Spikes on side 0
    g_arena.add_toxic_spikes(0, 1000.0f, 800.0f);
    HazardType hz = HAZARD_NONE;
    bool col = g_arena.check_hazard_collision(1000.0f, 800.0f, 0.0f, 0, hz);
    assert(col && hz == HAZARD_TOXIC_SPIKES);

    // Grounded non-Poison, non-Steel switch-in (e.g. Pikachu) gets Poisoned
    Fighter test_pika_hazard;
    test_pika_hazard.init(0, PKMN_PIKACHU, 1000.0f, 800.0f, false);
    assert(test_pika_hazard.status == STATUS_NONE);
    test_pika_hazard.update(0.016f, &p2_duel);
    assert(test_pika_hazard.status == STATUS_POISON);

    // Airborne switch-in (e.g. Charizard at z = 30px) is unaffected
    Fighter test_chz_hazard;
    test_chz_hazard.init(0, PKMN_CHARIZARD, 1000.0f, 800.0f, false);
    test_chz_hazard.z = 30.0f;
    test_chz_hazard.update(0.016f, &p2_duel);
    assert(test_chz_hazard.status == STATUS_NONE);

    // Steel type switch-in (e.g. Lucario is Fighting/Steel) is immune to poison
    Fighter test_lucario_hazard;
    test_lucario_hazard.init(0, PKMN_LUCARIO, 1000.0f, 800.0f, false);
    test_lucario_hazard.update(0.016f, &p2_duel);
    assert(test_lucario_hazard.status == STATUS_NONE);
    printf("  -> PASS: Toxic Spikes poison grounded targets, spare airborne flyers, and are blocked by Steel typing.\n\n");

    // -----------------------------------------------------------------
    // TEST 6: Status DoT Damage Ticks & Damage Lag Bar Drainage
    // -----------------------------------------------------------------
    printf("[TEST 6] Verifying Status DoT Damage Ticks & Lag Bar Drainage...\n");
    Fighter psn_target;
    psn_target.init(0, PKMN_PIKACHU, 500.0f, 500.0f, false);
    psn_target.apply_status(STATUS_POISON);
    int init_hp = psn_target.hp;

    // Advance 3.1 seconds to trigger DoT tick
    for (int frame = 0; frame < 190; ++frame) {
        psn_target.update(0.0166f, &p2_duel);
    }
    assert(psn_target.hp < init_hp);
    assert(psn_target.display_lag_hp >= (float)psn_target.hp);
    printf("  -> PASS: Poison 4%% max HP DoT successfully ticked down HP with smooth display lag drainage.\n\n");

    // -----------------------------------------------------------------
    // TEST 7: Charizard Blaze Trigger at <= 30% HP
    // -----------------------------------------------------------------
    printf("[TEST 7] Verifying Charizard Blaze Trigger at <= 30%% HP...\n");
    Fighter charizard_blaze;
    charizard_blaze.init(0, PKMN_CHARIZARD, 500.0f, 500.0f, false);
    assert(!charizard_blaze.blaze_active);

    // Reduce Charizard to 25% HP (threshold is <= 30%)
    charizard_blaze.hp = (int)((float)charizard_blaze.max_hp * 0.25f);
    charizard_blaze.update(0.0166f, &p2_duel);
    assert(charizard_blaze.blaze_active == true);
    printf("  -> PASS: Charizard Blaze successfully triggers at <= 30%% HP.\n\n");

    // -----------------------------------------------------------------
    // TEST 8: Move VFX Pipeline: Motion Blur Afterimages & Impact Decals
    // -----------------------------------------------------------------
    printf("[TEST 8] Verifying Move VFX: Motion Smear Afterimages & Impact Decals...\n");
    Fighter rush_fighter;
    rush_fighter.init(0, PKMN_PIKACHU, 500.0f, 500.0f, false);
    rush_fighter.state = STATE_ATTACK;
    rush_fighter.active_move = MOVE_VOLT_TACKLE;
    rush_fighter.attack_timer = 0.5f;
    rush_fighter.ghost_timer = 0.04f; // Trigger ghost spawn threshold (>= 0.038f)

    rush_fighter.update(0.016f, &p2_duel);

    bool ghost_found = false;
    for (int g = 0; g < 8; ++g) {
        if (rush_fighter.dash_ghosts[g].alpha > 0.0f) {
            ghost_found = true;
            assert(rush_fighter.dash_ghosts[g].alpha >= 100.0f);
            break;
        }
    }
    assert(ghost_found == true);

    // Verify Quick Attack also triggers motion smear afterimages
    Fighter qa_fighter;
    qa_fighter.init(0, PKMN_PIKACHU, 400.0f, 400.0f, false);
    qa_fighter.state = STATE_ATTACK;
    qa_fighter.active_move = MOVE_QUICK_ATTACK;
    qa_fighter.attack_timer = 0.24f;
    qa_fighter.ghost_timer = 0.04f;
    qa_fighter.update(0.016f, &p2_duel);

    bool qa_ghost = false;
    for (int g = 0; g < 8; ++g) {
        if (qa_fighter.dash_ghosts[g].alpha > 0.0f) {
            qa_ghost = true;
            break;
        }
    }
    assert(qa_ghost == true);

    // Verify Bone Rush also triggers motion smear afterimages
    Fighter br_fighter;
    br_fighter.init(0, PKMN_LUCARIO, 400.0f, 400.0f, false);
    br_fighter.state = STATE_ATTACK;
    br_fighter.active_move = MOVE_BONE_RUSH;
    br_fighter.attack_timer = 0.28f;
    br_fighter.ghost_timer = 0.04f;
    br_fighter.update(0.016f, &p2_duel);

    bool br_ghost = false;
    for (int g = 0; g < 8; ++g) {
        if (br_fighter.dash_ghosts[g].alpha > 0.0f) {
            br_ghost = true;
            break;
        }
    }
    assert(br_ghost == true);

    // Verify all 4 persistent impact decal types
    g_arena.clear_decals();
    assert(g_arena.decals[0].active == false);

    g_arena.add_decal(DECAL_SCORCH_MARK, 600.0f, 600.0f, 45.0f);
    assert(g_arena.decals[0].active == true);
    assert(g_arena.decals[0].type == DECAL_SCORCH_MARK);
    assert(g_arena.decals[0].radius == 45.0f);

    g_arena.add_decal(DECAL_IMPACT_CRATER, 650.0f, 650.0f, 38.0f);
    assert(g_arena.decals[1].active == true);
    assert(g_arena.decals[1].type == DECAL_IMPACT_CRATER);

    g_arena.add_decal(DECAL_SAND_FURROW, 700.0f, 700.0f, 32.0f);
    assert(g_arena.decals[2].active == true);
    assert(g_arena.decals[2].type == DECAL_SAND_FURROW);

    g_arena.add_decal(DECAL_TECTONIC_CRACK, 750.0f, 750.0f, 120.0f);
    assert(g_arena.decals[3].active == true);
    assert(g_arena.decals[3].type == DECAL_TECTONIC_CRACK);

    printf("  -> PASS: Motion blur afterimages spawn during rush moves (Volt Tackle, Quick Attack, Bone Rush), and all 4 decal types persist on the battlefield.\n\n");

    // -----------------------------------------------------------------
    // TEST 9: Agility & Double Team Animation & Physics Lifecycle
    // -----------------------------------------------------------------
    {
        printf("[TEST 9] Verifying Agility & Double Team Animation Mechanics...\n");
        Fighter pika;
        pika.init(0, PKMN_PIKACHU, 500.0f, 500.0f, false);

        // Execute Agility
        MoveRegistry::execute(MOVE_AGILITY, &pika, nullptr);
        assert(pika.agility_timer > 3.0f);
        assert(pika.active_move == MOVE_AGILITY);

        // Update with movement input to verify ghost generation during Agility
        pika.input_dx = 1.0f;
        pika.input_dy = 0.0f;
        pika.ghost_timer = 0.05f;
        float prev_agility_timer = pika.agility_timer;
        pika.update(0.016f, &p2_duel);
        assert(pika.agility_timer < prev_agility_timer); // Decrementing properly!

        bool agility_ghost_found = false;
        for (int g = 0; g < 8; ++g) {
            if (pika.dash_ghosts[g].alpha > 0.0f) {
                agility_ghost_found = true;
                break;
            }
        }
        assert(agility_ghost_found == true);

        // Execute Double Team
        pika.agility_timer = 0.0f;
        MoveRegistry::execute(MOVE_DOUBLE_TEAM, &pika, nullptr);
        assert(pika.double_team_timer > 4.0f);
        for (int i = 0; i < Fighter::MAX_CLONES; ++i) {
            assert(pika.clones[i].active == true);
        }
        float initial_clone_x = pika.clones[0].x;
        pika.update(0.10f, &p2_duel);
        assert(pika.clone_orbit_angle > 0.0f);
        assert(pika.clones[0].x != initial_clone_x); // Orbiting positions update!

        // Fast-forward past expiry
        pika.update(4.5f, &p2_duel);
        assert(pika.double_team_timer == 0.0f);
        for (int i = 0; i < Fighter::MAX_CLONES; ++i) {
            assert(pika.clones[i].active == false); // Cleanly cleared!
        }
        printf("  -> PASS: Agility timer decrement & ghost generation verified; Double Team clones orbit and expire cleanly.\n\n");
    }

    // -----------------------------------------------------------------
    // TEST 10: Seismic Toss Stratosphere Orbit, Slam Damage & Unfreeze
    // -----------------------------------------------------------------
    {
        printf("[TEST 10] Verifying Seismic Toss Aerial Hold, Slam Damage & Release...\n");
        Fighter charizard_st, victim;
        charizard_st.init(0, PKMN_CHARIZARD, 500.0f, 500.0f, false);
        victim.init(1, PKMN_PIKACHU, 540.0f, 500.0f, true);
        int initial_victim_hp = victim.hp;

        // Simulate grab connection
        victim.is_grabbed = true;
        victim.grabbed_by_id = charizard_st.id;
        victim.state = STATE_HITSTUN;
        assert(!victim.can_act()); // Victim is frozen while grabbed

        charizard_st.active_move = MOVE_SEISMIC_TOSS;
        charizard_st.state = STATE_ATTACK;
        charizard_st.move_phase = 1;
        charizard_st.attack_duration = 0.95f;
        charizard_st.attack_timer = 0.95f;
        charizard_st.seismic_toss_center_x = 520.0f;
        charizard_st.seismic_toss_center_y = 500.0f;

        // Advance 25 frames (~0.40s) into orbit
        for (int frame = 0; frame < 25; ++frame) {
            charizard_st.update(0.0166f, &victim);
        }
        assert(charizard_st.z > 50.0f);
        assert(victim.z == charizard_st.z); // Victim lifted into sky with Charizard!
        assert(victim.x == charizard_st.x);
        assert(victim.y == charizard_st.y);
        assert(victim.is_grabbed == true);

        // Complete the slam onto ground (advance remaining 40 frames)
        g_arena.clear_decals();
        for (int frame = 0; frame < 40; ++frame) {
            charizard_st.update(0.0166f, &victim);
        }
        assert(charizard_st.active_move == MOVE_NONE);
        assert(victim.is_grabbed == false);
        assert(victim.grabbed_by_id == -1);
        assert(victim.z == 0.0f);
        assert(victim.hp < initial_victim_hp); // Heavy slam damage dealt!
        assert(victim.state == STATE_HITSTUN); // Hitstun applied
        for (int frame = 0; frame < 40; ++frame) {
            victim.update(0.0166f, &charizard_st); // After hitstun elapses
        }
        assert(victim.can_act()); // Victim can act again - NOT permanently frozen!
        assert(g_arena.decals[0].active == true);
        assert(g_arena.decals[0].type == DECAL_IMPACT_CRATER); // Impact crater spawned
        printf("  -> PASS: Seismic Toss lifts opponent into 3D airspace, deals slam damage, creates crater, and releases target.\n\n");
    }

    // -----------------------------------------------------------------
    // TEST 11: Lucario Full 8-Move Roster Availability
    // -----------------------------------------------------------------
    {
        printf("[TEST 11] Verifying Lucario Canonical 8-Move Pool...\n");
        Fighter lucario;
        lucario.init(0, PKMN_LUCARIO, 500.0f, 500.0f, false);
        assert(lucario.movepool_count == 8);
        bool has_force_palm = false, has_aura_sphere = false, has_close_combat = false, has_extreme_speed = false;
        bool has_bone_rush = false, has_flash_cannon = false, has_dragon_pulse = false, has_protect = false;
        for (int m = 0; m < lucario.movepool_count; ++m) {
            if (lucario.movepool[m] == MOVE_FORCE_PALM)    has_force_palm = true;
            if (lucario.movepool[m] == MOVE_AURA_SPHERE)   has_aura_sphere = true;
            if (lucario.movepool[m] == MOVE_CLOSE_COMBAT)  has_close_combat = true;
            if (lucario.movepool[m] == MOVE_EXTREME_SPEED) has_extreme_speed = true;
            if (lucario.movepool[m] == MOVE_BONE_RUSH)     has_bone_rush = true;
            if (lucario.movepool[m] == MOVE_FLASH_CANNON)  has_flash_cannon = true;
            if (lucario.movepool[m] == MOVE_DRAGON_PULSE)  has_dragon_pulse = true;
            if (lucario.movepool[m] == MOVE_PROTECT)       has_protect = true;
        }
        assert(has_force_palm && has_aura_sphere && has_close_combat && has_extreme_speed &&
               has_bone_rush && has_flash_cannon && has_dragon_pulse && has_protect);
        printf("  -> PASS: Lucario has all 8 canonical moves in its draft pool.\n\n");
    }

    // -----------------------------------------------------------------
    // TEST 12: Direct Line-of-Sight Aim Tracking
    // -----------------------------------------------------------------
    {
        printf("[TEST 12] Verifying Direct Line-of-Sight Aim Tracking...\n");
        Fighter cpu;
        cpu.init(1, PKMN_CHARIZARD, 500.0f, 500.0f, true);
        Fighter moving_player;
        moving_player.init(0, PKMN_PIKACHU, 800.0f, 500.0f, false);
        moving_player.vx = 0.0f;
        moving_player.vy = 320.0f; // Moving South at 320 px/s

        BattleAI ai_test;
        ai_test.init(AI_MODE_BOSS);
        ai_test.update(&cpu, &moving_player, 0.016f);

        // Aim angle must directly target current player position (800, 500), angle 0.0 rad (due East)
        float expected_angle = atan2f(moving_player.y - cpu.y, moving_player.x - cpu.x);
        assert(fabsf(cpu.aim_angle - expected_angle) < 0.001f);
        printf("  -> PASS: CPU directly tracks target coordinate without overshooting or lateral lead distortion.\n\n");
    }

    // -----------------------------------------------------------------
    // TEST 13: Hit-Confirm True Combo Linking on Stunned Target
    // -----------------------------------------------------------------
    {
        printf("[TEST 13] Verifying Hit-Confirm True Combo Linking...\n");
        Fighter lucario_cpu;
        lucario_cpu.init(1, PKMN_LUCARIO, 500.0f, 500.0f, true);
        Fighter stunned_target;
        stunned_target.init(0, PKMN_PIKACHU, 550.0f, 500.0f, false);
        stunned_target.state = STATE_HITSTUN;
        stunned_target.hitstun_timer = 0.35f;

        BattleAI ai_test;
        ai_test.init(AI_MODE_BOSS);
        ai_test.attack_cooldown_timer = 0.50f; // In neutral cooldown, but hit-confirm should bypass!

        bool combo_executed = ai_test.execute_hit_confirm_combo(&lucario_cpu, &stunned_target, 50.0f, stunned_target.x, stunned_target.y);
        assert(combo_executed == true);
        assert(lucario_cpu.state == STATE_ATTACK);
        assert(lucario_cpu.active_move == MOVE_FORCE_PALM);
        assert(ai_test.combo_stage == 1);
        printf("  -> PASS: Hit-Confirm bypassed attack cooldown and chained into Force Palm follow-up on stunned opponent.\n\n");
    }

    // -----------------------------------------------------------------
    // TEST 14: Whiff Punish Trigger on Opponent End-Lag
    // -----------------------------------------------------------------
    {
        printf("[TEST 14] Verifying Whiff Punish on Opponent End-Lag...\n");
        Fighter pika_cpu;
        pika_cpu.init(1, PKMN_PIKACHU, 500.0f, 500.0f, true);
        Fighter whiffing_player;
        whiffing_player.init(0, PKMN_CHARIZARD, 680.0f, 500.0f, false);
        whiffing_player.state = STATE_ATTACK;
        whiffing_player.active_move = MOVE_SEISMIC_TOSS;
        whiffing_player.attack_timer = 0.25f; // Locked in whiff recovery

        BattleAI ai_punish;
        ai_punish.init(AI_MODE_BOSS);

        bool punish_executed = ai_punish.evaluate_whiff_punish(&pika_cpu, &whiffing_player, 180.0f);
        assert(punish_executed == true);
        assert(pika_cpu.state == STATE_ATTACK);
        assert(pika_cpu.active_move == MOVE_QUICK_ATTACK || pika_cpu.active_move == MOVE_VOLT_TACKLE);
        printf("  -> PASS: Whiff punish instantly triggered rush attack against opponent locked in move end-lag.\n\n");
    }

    // -----------------------------------------------------------------
    // TEST 15: Tactical Stance Transitions & Rushdown Earthquake Execution
    // -----------------------------------------------------------------
    {
        printf("[TEST 15] Verifying Tactical Stance Transitions & Rushdown Earthquake Execution...\n");
        Fighter charizard_cpu;
        charizard_cpu.init(1, PKMN_CHARIZARD, 500.0f, 500.0f, true);
        charizard_cpu.equip_move(0, MOVE_SCORCHING_SANDS); // Slot 0
        charizard_cpu.equip_move(1, MOVE_FLAMETHROWER);    // Slot 1
        charizard_cpu.equip_move(2, MOVE_EARTHQUAKE);      // Slot 2
        charizard_cpu.equip_move(3, MOVE_SEISMIC_TOSS);    // Slot 3
        charizard_cpu.stamina = 120.0f;

        Fighter pika_target;
        pika_target.init(0, PKMN_PIKACHU, 650.0f, 500.0f, false); // 150px away (inside 180px Earthquake radius!)

        BattleAI ai_stance;
        ai_stance.init(AI_MODE_BOSS);
        ai_stance.stance = AI_STANCE_RUSHDOWN;
        ai_stance.stance_timer = 3.0f;
        ai_stance.attack_cooldown_timer = 0.0f;

        // Run one update frame
        ai_stance.update(&charizard_cpu, &pika_target, 0.016f);

        // In Rushdown stance at 150px, Earthquake (Slot 2) MUST be executed instead of getting starved by Slot 0!
        assert(charizard_cpu.state == STATE_ATTACK);
        assert(charizard_cpu.active_move == MOVE_EARTHQUAKE);
        printf("  -> PASS: Rushdown stance prioritized close-range Earthquake (Slot 2) over Slot 0 ranged poke.\n");

        // Verify stamina depletion triggers transition to recharge stance
        charizard_cpu.stamina = 18.0f; // Critical low stamina
        ai_stance.update(&charizard_cpu, &pika_target, 0.016f);
        assert(ai_stance.stance == AI_STANCE_RECHARGE);
        assert(ai_stance.stance_timer > 0.0f);
        printf("  -> PASS: Low stamina (< 25 SP) correctly forced transition to AI_STANCE_RECHARGE.\n\n");
    }

    // -----------------------------------------------------------------
    // TEST 16: Pikachu Signature Move: Thunder Execution, Telegraph & 3D Aerial Reach
    // -----------------------------------------------------------------
    {
        printf("[TEST 16] Verifying Pikachu Signature Move: Thunder Execution, Telegraph & 3D Aerial Reach...\n");
        g_projectiles.init();

        const MoveDef& thunder_def = MoveRegistry::get(MOVE_THUNDER);
        assert(thunder_def.id == MOVE_THUNDER);
        assert(std::string(thunder_def.name) == "Thunder");
        assert(thunder_def.element_type == TYPE_ELECTRIC);
        assert(thunder_def.category == CATEGORY_SPECIAL);
        assert(thunder_def.base_power == 36);
        assert(thunder_def.stamina_cost == 40);
        assert(thunder_def.is_targeted == true);
        printf("  -> PASS: MoveRegistry metadata correctly verified (36 BP, 40 SP, targeted sky column).\n");

        Fighter pika;
        pika.init(0, PKMN_PIKACHU, 400.0f, 400.0f, false);
        assert(pika.abilities[2].move_id == MOVE_THUNDER);
        assert(pika.stamina == 100.0f);

        Fighter airborne_charizard;
        airborne_charizard.init(1, PKMN_CHARIZARD, 550.0f, 400.0f, true);
        airborne_charizard.z = 150.0f; // Airborne flyer high above ground (z = 150px)
        int initial_hp = airborne_charizard.hp;

        // Pikachu uses Thunder on airborne Charizard (target at 550, 400)
        pika.use_ability(2, &airborne_charizard, 550.0f, 400.0f);
        assert(pika.stamina == 60.0f); // 40 SP consumed
        assert(pika.state == STATE_ATTACK);
        assert(pika.active_move == MOVE_THUNDER);

        // Find spawned PROJ_THUNDER
        Projectile* thunder_proj = nullptr;
        for (int i = 0; i < MAX_PROJ; ++i) {
            if (g_projectiles.pool[i].active && g_projectiles.pool[i].type == PROJ_THUNDER) {
                thunder_proj = &g_projectiles.pool[i];
                break;
            }
        }
        assert(thunder_proj != nullptr);
        assert(thunder_proj->delay_timer > 0.30f);
        assert(thunder_proj->can_damage == false);
        assert(thunder_proj->z_max >= 300.0f); // Vertical column reaches sky targets!

        // Frame 1: During telegraph (0.15s elapsed), zero damage must be dealt
        g_projectiles.update(0.15f, pika.x, pika.y, airborne_charizard.x, airborne_charizard.y);
        assert(thunder_proj->can_damage == false);
        bool hit_early = pika.check_hits(&airborne_charizard);
        assert(!hit_early);
        assert(airborne_charizard.hp == initial_hp);
        printf("  -> PASS: Telegraph window (0.32s) safely guarantees zero premature damage.\n");

        // Frame 2: Advance past telegraph (0.20s elapsed -> total 0.35s > 0.32s)
        g_projectiles.update(0.20f, pika.x, pika.y, airborne_charizard.x, airborne_charizard.y);
        assert(thunder_proj->has_struck == true);
        assert(thunder_proj->can_damage == true);

        // Hit resolution: Thunder sky pillar strikes airborne target at z = 150px!
        bool hit_landed = pika.check_hits(&airborne_charizard);
        assert(hit_landed == true);
        assert(airborne_charizard.hp < initial_hp);
        assert(g_projectiles.screen_shake >= 0.30f); // Severe screen shake triggered
        printf("  -> PASS: Celestial sky pillar struck airborne Charizard (z = 150px) for %d damage with screen shake.\n\n",
               initial_hp - airborne_charizard.hp);
    }

    // -----------------------------------------------------------------
    // TEST 17: Overhauled Thunderbolt (Anchored Fractal Beam Segment Collision)
    //          & Flamethrower (Continuous Channeled Stream & Expanding Vortices)
    // -----------------------------------------------------------------
    {
        printf("[TEST 17] Verifying Overhauled Thunderbolt Continuous Beam & Flamethrower Stream...\n");
        g_projectiles.init();

        // Part A: Thunderbolt Anchored Lightning Beam & Segment Collision
        Fighter pika;
        pika.init(0, PKMN_PIKACHU, 300.0f, 300.0f, false);
        pika.aim_angle = 0.0f; // Aiming right along +X
        pika.equip_move(1, MOVE_THUNDERBOLT);

        Fighter dummy_charizard;
        dummy_charizard.init(1, PKMN_CHARIZARD, 480.0f, 300.0f, true); // 180px away (directly in beam path)
        int initial_charizard_hp = dummy_charizard.hp;

        // Pikachu fires Thunderbolt
        pika.use_ability(1, &dummy_charizard, 480.0f, 300.0f);
        assert(pika.state == STATE_ATTACK);
        assert(pika.active_move == MOVE_THUNDERBOLT);
        assert(pika.attack_duration >= 0.35f);

        // Find spawned PROJ_THUNDERBOLT
        Projectile* tb_proj = nullptr;
        for (int i = 0; i < MAX_PROJ; ++i) {
            if (g_projectiles.pool[i].active && g_projectiles.pool[i].type == PROJ_THUNDERBOLT) {
                tb_proj = &g_projectiles.pool[i];
                break;
            }
        }
        assert(tb_proj != nullptr);
        assert(tb_proj->piercing == true);
        assert(fabsf(tb_proj->target_x - (pika.x + 24.0f)) < 1.0f); // Anchor matches cheek origin!

        // Advance 0.12s: beam front advances past dummy Charizard to ~500px+
        g_projectiles.update(0.12f, pika.x, pika.y, dummy_charizard.x, dummy_charizard.y);
        MoveRegistry::update_active_move(&pika, 0.12f, &dummy_charizard);

        // Segment collision check: Must detect hit along beam line segment even if front has passed!
        bool tb_hit = pika.check_hits(&dummy_charizard);
        assert(tb_hit == true);
        assert(dummy_charizard.hp < initial_charizard_hp);
        assert(g_projectiles.screen_shake >= 0.15f);
        assert(tb_proj->hit_cooldown > 0); // Multi-hit tick cooldown engaged
        printf("  -> PASS: Thunderbolt continuous beam segment collision hit target midway along beam (180px away).\n");

        // Advance further and verify max beam range clamp (440px)
        for (int step = 0; step < 10; ++step) {
            g_projectiles.update(0.03f, pika.x, pika.y, dummy_charizard.x, dummy_charizard.y);
        }
        float beam_length = sqrtf((tb_proj->x - tb_proj->target_x) * (tb_proj->x - tb_proj->target_x) +
                                  (tb_proj->y - tb_proj->target_y) * (tb_proj->y - tb_proj->target_y));
        assert(beam_length <= 441.0f); // Clamped at 440px max range
        printf("  -> PASS: Thunderbolt beam front successfully clamped at maximum range (%0.1fpx <= 440px).\n", beam_length);

        // Part B: Flamethrower Continuous Stream Channeling & Dynamic Puff Expansion
        g_projectiles.init();
        Fighter charizard;
        charizard.init(0, PKMN_CHARIZARD, 400.0f, 400.0f, false);
        charizard.aim_angle = 0.0f; // Aiming right along +X
        charizard.equip_move(1, MOVE_FLAMETHROWER);

        Fighter dummy_lucario;
        dummy_lucario.init(1, PKMN_LUCARIO, 540.0f, 400.0f, true); // 140px away
        int initial_lucario_hp = dummy_lucario.hp;

        // Charizard unleashes Flamethrower
        charizard.use_ability(1, &dummy_lucario, 540.0f, 400.0f);
        assert(charizard.state == STATE_ATTACK);
        assert(charizard.active_move == MOVE_FLAMETHROWER);
        assert(charizard.attack_duration >= 0.35f);

        // Step active move and simulation through 4 update frames (0.14s)
        for (int f = 0; f < 4; ++f) {
            MoveRegistry::update_active_move(&charizard, 0.036f, &dummy_lucario);
            g_projectiles.update(0.036f, charizard.x, charizard.y, dummy_lucario.x, dummy_lucario.y);
        }

        // Count active Flamethrower puffs in flight
        int flame_puff_count = 0;
        float max_flame_radius = 0.0f;
        for (int i = 0; i < MAX_PROJ; ++i) {
            if (g_projectiles.pool[i].active && g_projectiles.pool[i].type == PROJ_FLAMETHROWER) {
                flame_puff_count++;
                if (g_projectiles.pool[i].radius > max_flame_radius) {
                    max_flame_radius = g_projectiles.pool[i].radius;
                }
            }
        }
        assert(flame_puff_count >= 3); // Channeled stream continuously emitted multiple overlapping puffs!
        assert(max_flame_radius > 12.0f); // Puffs dynamically expanded as they traveled!
        printf("  -> PASS: Flamethrower channeled emission generated %d overlapping vortices expanding up to %0.1fpx radius.\n",
               flame_puff_count, max_flame_radius);

        // Verify hit connection and micro-hitstop on target
        bool flame_hit = charizard.check_hits(&dummy_lucario);
        assert(flame_hit == true);
        assert(dummy_lucario.hp < initial_lucario_hp);
        assert(g_projectiles.hitstop_timer > 0.0f); // 2-frame micro-hitstop triggered
        printf("  -> PASS: Flamethrower multi-hit stream connected on Lucario with micro-hitstop and thermal pushback.\n\n");
    }

    // -----------------------------------------------------------------
    // TEST 18: BattleAI Fire Spin Residue Avoidance & Hazard Repulsion
    // -----------------------------------------------------------------
    {
        printf("[TEST 18] Verifying BattleAI Fire Spin Residue Avoidance & Hazard Repulsion...\n");
        g_projectiles.init();

        // 1. Spawn a landed, stationary Fire Spin residue vortex at (500, 500)
        g_projectiles.spawn(1, PROJ_FIRE_SPIN, 500.0f, 500.0f, 0.0f, 0.0f, 10, 3.0f, 500.0f, 500.0f);
        Projectile* fire_spin = nullptr;
        for (int i = 0; i < MAX_PROJ; ++i) {
            if (g_projectiles.pool[i].active && g_projectiles.pool[i].type == PROJ_FIRE_SPIN) {
                fire_spin = &g_projectiles.pool[i];
                break;
            }
        }
        assert(fire_spin != nullptr);
        fire_spin->has_struck = true;
        fire_spin->radius = 52.0f;

        // 2. Setup Pikachu AI at (420, 500) - 80px away from center, inside the 117px danger zone
        Fighter pika_ai;
        pika_ai.init(0, PKMN_PIKACHU, 420.0f, 500.0f, false);
        pika_ai.stamina = 100.0f;

        // Opponent Charizard is on the opposite side at (650, 500)
        Fighter charizard_target;
        charizard_target.init(1, PKMN_CHARIZARD, 650.0f, 500.0f, true);

        BattleAI ai;
        ai.init(AI_MODE_BOSS);
        ai.stance = AI_STANCE_NEUTRAL;

        // Run AI update for 1 frame
        ai.update(&pika_ai, &charizard_target, 0.016f);

        // Normally, approaching Charizard at (650, 500) would demand +X direction (input_dx > 0)
        // BUT Fire Spin residue is at (500, 500) directly between them!
        // The hazard repulsion and tangential deflection must steer Pikachu AWAY from +X into negative X or tangential Y!
        assert(pika_ai.input_dx < 0.0f || fabsf(pika_ai.input_dy) > 0.5f);
        printf("  -> PASS: Pikachu AI successfully deflected movement away from Fire Spin residue.\n");

        // 3. Setup critical proximity: Pikachu placed at (475, 500) - 25px away from epicenter
        pika_ai.x = 475.0f;
        pika_ai.y = 500.0f;
        ai.update(&pika_ai, &charizard_target, 0.016f);
        // Emergency escape MUST point directly away from hazard (-X direction)
        assert(pika_ai.input_dx < -0.5f);
        printf("  -> PASS: Emergency escape vector pushed Pikachu directly outward from fire epicenter (dx = %0.2f).\n\n",
               pika_ai.input_dx);
    }

    // =========================================================================
    // TEST 19: Greninja (#0658) Species, RPG Stats, Ninjutsu Moves & AI Verification
    // =========================================================================
    {
        printf("[TEST 19] Verifying Greninja (#0658) Stats, Ninjutsu Moves & AI...\n");
        g_projectiles.init();

        Fighter greninja;
        greninja.init(0, PKMN_GRENINJA, 600.0f, 600.0f, false);

        // 1. Dual Typing Verification
        assert(greninja.type1 == TYPE_WATER);
        assert(greninja.type2 == TYPE_DARK);
        assert(greninja.base_stats.hp == 540);
        assert(greninja.base_stats.speed == 122);
        assert(greninja.move_speed > 330.0f); // 332.28 px/s top ninja speed
        printf("  -> PASS: Greninja dual typing (Water/Dark), 540 HP, base speed 122 (%0.2f px/s).\n", greninja.move_speed);

        // 2. Default Moveset Verification
        assert(greninja.abilities[0].move_id == MOVE_WATER_SHURIKEN);
        assert(greninja.abilities[1].move_id == MOVE_NIGHT_SLASH);
        assert(greninja.abilities[2].move_id == MOVE_HYDRO_PUMP);
        assert(greninja.abilities[3].move_id == MOVE_AERIAL_ACE);
        printf("  -> PASS: Equipped 4 signature moves (Water Shuriken, Night Slash, Hydro Pump, Aerial Ace).\n");

        // 3. Water Shuriken Multi-Hit Sequential Launch
        Fighter target;
        target.init(1, PKMN_CHARIZARD, 850.0f, 600.0f, true);
        greninja.aim_angle = 0.0f; // Aim directly East (+X)
        greninja.stamina = 100.0f;

        greninja.use_ability(0, &target, target.x, target.y);
        assert(greninja.active_move == MOVE_WATER_SHURIKEN);

        int shuriken_count = 0;
        for (int i = 0; i < MAX_PROJ; ++i) {
            if (g_projectiles.pool[i].active && g_projectiles.pool[i].type == PROJ_WATER_SHURIKEN) {
                shuriken_count++;
            }
        }
        assert(shuriken_count == 1);

        // Advance by 0.10s -> second shuriken launched
        MoveRegistry::update_active_move(&greninja, 0.10f, &target);
        shuriken_count = 0;
        for (int i = 0; i < MAX_PROJ; ++i) {
            if (g_projectiles.pool[i].active && g_projectiles.pool[i].type == PROJ_WATER_SHURIKEN) {
                shuriken_count++;
            }
        }
        assert(shuriken_count == 2);

        // Advance by another 0.10s -> third shuriken launched
        MoveRegistry::update_active_move(&greninja, 0.10f, &target);
        shuriken_count = 0;
        for (int i = 0; i < MAX_PROJ; ++i) {
            if (g_projectiles.pool[i].active && g_projectiles.pool[i].type == PROJ_WATER_SHURIKEN) {
                shuriken_count++;
            }
        }
        assert(shuriken_count == 3);
        printf("  -> PASS: Water Shuriken multi-hit sequential release successfully spawned 3 aqua stars.\n");

        // 4. Aerial Ace Z-Axis Evasive Leap
        g_projectiles.init();
        greninja.state = STATE_IDLE;
        greninja.attack_timer = 0.0f;
        greninja.abilities[3].cooldown_timer = 0.0f;
        greninja.stamina = 100.0f;

        greninja.use_ability(3, &target, target.x, target.y);
        assert(greninja.active_move == MOVE_AERIAL_ACE);
        MoveRegistry::update_active_move(&greninja, 0.15f, &target); // Mid-flight apex
        assert(greninja.z > 20.0f); // Airborne above ground hazards!
        printf("  -> PASS: Aerial Ace aerodynamic sonic leap achieves vertical elevation (z = %0.1f px).\n", greninja.z);

        // 5. Hit-Cancel Window on Night Slash
        g_projectiles.init();
        greninja.state = STATE_IDLE;
        greninja.attack_timer = 0.0f;
        greninja.z = 0.0f;
        greninja.abilities[1].cooldown_timer = 0.0f;
        greninja.stamina = 100.0f;

        target.x = greninja.x + 35.0f; // Point-blank
        target.y = greninja.y;
        target.invuln_timer = 0.0f;

        greninja.use_ability(1, &target, target.x, target.y);
        assert(greninja.active_move == MOVE_NIGHT_SLASH);

        // Resolve collision
        bool hit = greninja.check_hits(&target);
        assert(hit);
        assert(greninja.hit_cancel_timer > 0.0f);
        printf("  -> PASS: Night Slash impact successfully activated 0.18s hit-cancel combo window.\n");

        // 6. AI Hit-Confirm Combo Execution
        BattleAI ai;
        ai.init(AI_MODE_BOSS);
        target.state = STATE_HITSTUN;
        target.hitstun_timer = 0.40f;
        target.x = greninja.x + 120.0f;
        greninja.state = STATE_IDLE;
        greninja.attack_timer = 0.0f;
        greninja.stamina = 100.0f;
        greninja.abilities[1].cooldown_timer = 0.0f;
        greninja.abilities[3].cooldown_timer = 0.0f;

        bool combo_exec = ai.execute_hit_confirm_combo(&greninja, &target, 120.0f, target.x, target.y);
        assert(combo_exec);
        assert(greninja.active_move == MOVE_AERIAL_ACE || greninja.active_move == MOVE_NIGHT_SLASH);
        printf("  -> PASS: Greninja AI executed hit-confirm combo chain into confirmed hitstun target.\n\n");
    }

    // -----------------------------------------------------------------
    // TEST 20: Verifying Overhauled Greninja Move Visuals, Melee Clashes & Frame Pacing
    // -----------------------------------------------------------------
    {
        printf("[TEST 20] Verifying Overhauled Greninja Move Visuals, Melee Clashes & Frame Pacing...\n");
        g_projectiles.init();

        Fighter greninja;
        greninja.init(0, PKMN_GRENINJA, 300.0f, 300.0f, false);
        greninja.aim_angle = 0.0f;
        Fighter target;
        target.init(1, PKMN_CHARIZARD, 420.0f, 300.0f, true);

        // 1. Hydro Pump Fluid Expansion & Knockback
        greninja.equip_move(2, MOVE_HYDRO_PUMP);
        greninja.use_ability(2, &target, target.x, target.y);
        assert(greninja.active_move == MOVE_HYDRO_PUMP);

        Projectile* hp_proj = nullptr;
        for (int i = 0; i < MAX_PROJ; ++i) {
            if (g_projectiles.pool[i].active && g_projectiles.pool[i].type == PROJ_HYDRO_PUMP) {
                hp_proj = &g_projectiles.pool[i];
                break;
            }
        }
        assert(hp_proj != nullptr);
        float init_r = hp_proj->radius;
        // Simulate forward 0.20s
        g_projectiles.update(0.20f, greninja.x, greninja.y, target.x, target.y);
        assert(hp_proj->radius > init_r); // Tsunami head expanding!
        printf("  -> PASS: Hydro Pump fluid radius dynamically expanded (%0.1f px -> %0.1f px).\n", init_r, hp_proj->radius);

        target.x = hp_proj->x;
        target.y = hp_proj->y;
        bool hp_hit = greninja.check_hits(&target);
        assert(hp_hit);
        assert(g_projectiles.screen_shake >= 0.30f);
        assert(target.vx >= 300.0f); // Heavy hydraulic pushback!
        printf("  -> PASS: Hydro Pump connected with screen shake (%0.2f) and heavy pushback (%0.1f px/s).\n",
               g_projectiles.screen_shake, target.vx);

        // 2. Night Slash Melee Clash
        g_projectiles.init();
        Fighter pika;
        pika.init(1, PKMN_PIKACHU, 380.0f, 300.0f, false);

        greninja.state = STATE_IDLE;
        greninja.attack_timer = 0.0f;
        greninja.active_move = MOVE_NONE;
        greninja.stamina = 100.0f;
        greninja.abilities[1].cooldown_timer = 0.0f;
        greninja.x = 300.0f; greninja.y = 300.0f; greninja.aim_angle = 0.0f;

        pika.state = STATE_IDLE;
        pika.attack_timer = 0.0f;
        pika.active_move = MOVE_NONE;
        pika.stamina = 100.0f;
        pika.abilities[3].cooldown_timer = 0.0f;
        pika.aim_angle = 3.14159f;

        greninja.equip_move(1, MOVE_NIGHT_SLASH);
        pika.equip_move(3, MOVE_VOLT_TACKLE);

        greninja.use_ability(1, &pika, pika.x, pika.y);
        pika.use_ability(3, &greninja, greninja.x, greninja.y);

        bool clash = g_projectiles.check_melee_clash(&greninja, &pika);
        assert(clash == true);
        printf("  -> PASS: Night Slash successfully engaged in Melee Clash against incoming Volt Tackle!\n");

        // 3. Sprite Keyframe Progression (12 PMD frames fully mapped)
        greninja.state = STATE_ATTACK;
        greninja.attack_duration = 0.30f;
        greninja.attack_timer = 0.15f; // Exactly 50% progression
        greninja.attack_sheet.num_frames = 12;
        greninja.update(0.001f, &target);
        assert(greninja.anim_frame == 6); // 50% * 12 = 6!
        printf("  -> PASS: Attack sprite keyframe normalized smoothly across full PMD sheet (frame 6 of 12).\n");

        // 4. Ghost Tinting on Rush Moves
        greninja.active_move = MOVE_NIGHT_SLASH;
        greninja.attack_timer = 0.20f;
        greninja.ghost_timer = 0.05f; // Trigger ghost spawn
        greninja.update(0.001f, &target);
        bool found_tinted_ghost = false;
        for (int i = 0; i < 8; ++i) {
            if (greninja.dash_ghosts[i].alpha > 0.0f && greninja.dash_ghosts[i].r == 147 && greninja.dash_ghosts[i].g == 51) {
                found_tinted_ghost = true;
                break;
            }
        }
        assert(found_tinted_ghost);
        printf("  -> PASS: Night Slash afterimage ghost tinted sinister dark violet (RGB 147, 51, 234).\n\n");
    }

    printf("=================================================================\n");
    printf("     ALL 20 AUTOMATED VERIFICATION TESTS PASSED WITH 100%% SUCCESS \n");
    printf("=================================================================\n");
    return 0;
}

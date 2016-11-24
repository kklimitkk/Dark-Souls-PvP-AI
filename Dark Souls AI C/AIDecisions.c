#include "AIDecisions.h"

void InstinctDecisionMaking(InstinctDecision* instinct_decision){
	instinct_decision->priority_decision = EnemyNeutral;

	float distanceByLine = distance(&Player, &Enemy);
	guiPrint(LocationJoystick",1:Distance:%f", distanceByLine);
	unsigned char AtkID = isAttackAnimation(Enemy.animationType_id);

	//Actions are organized in reverse order of importance
	//Higher actions are less important
	//TODO should formalize this in an actual order somehow

	//if not two handing
	if (!Player.twoHanding && distanceByLine > 5){
		instinct_decision->priority_decision = EnterAttackSubroutine;
		instinct_decision->attackid = TwoHandId;
	}
	//l hand bare handed, not holding shield. safety distance
	if (Player.l_weapon_id == 900000 && distanceByLine > 5){
		instinct_decision->priority_decision = EnterAttackSubroutine;
		instinct_decision->attackid = SwitchWeaponId;
	}
	//heal if enemy heals
	if ((Enemy.animationType_id == CrushUseItem || Enemy.animationType_id == EstusSwig_part1 || Enemy.animationType_id == EstusSwig_part2) && Player.hp < 2000){
		instinct_decision->priority_decision = EnterAttackSubroutine;
		instinct_decision->attackid = HealId;
	}

	//if enemy in range and we're not in invulnerable position (bs knockdown)
	if (distanceByLine <= Enemy.weaponRange && Player.animationType_id != KnockdownId){
		if (
			//if in an animation where subanimation is not used for hurtbox
			(AtkID == 3 && Enemy.subanimation <= AttackSubanimationActiveDuringHurtbox) ||
			//or animation where it is
			((AtkID == 2 || AtkID == 4) && (Enemy.subanimation >= AttackSubanimationWindupClosing && Enemy.subanimation <= AttackSubanimationActiveDuringHurtbox))
			//TODO and their attack will hit me(their rotation is correct and their weapon hitbox width is greater than their rotation delta)
			//&& (Phantom->rotation)>((Player->rotation) - 3.1) && (Phantom->rotation)<((Player->rotation) + 3.1)
			//TODO if enemy not in bs knockdown
			)
		{
			OverrideLowPrioritySubroutines();
			guiPrint(LocationDetection",0:about to be hit (anim type id:%d) (suban id:%d)", Enemy.animationType_id, Enemy.subanimation);
			instinct_decision->priority_decision = EnterDodgeSubroutine;

			//Decide on dodge action

			//if we got hit already, and are in a state we can't dodge from, toggle escape the next hit
			if (Player.subanimation == PoiseBrokenSubanimation && (Enemy.dodgeTimeRemaining > 0.2 && Enemy.dodgeTimeRemaining < 0.3)){
				instinct_decision->defenseid = ToggleEscapeId;
			}
			//while staggered, dont enter any subroutines
			if (Player.subanimation != PoiseBrokenSubanimation){
				//if the reverse roll is close enough to put us behind the enemy and we have enough windup time to reverse roll
				if (
					distance(&Player, &Enemy) <= 3 && TotalTimeInSectoReverseRoll < Enemy.dodgeTimeRemaining &&
					//if just reverse rolled and next incoming attack and weapon speed < ?, do normal roll
					(last_subroutine_states_self[0] != ReverseRollBSId || TotalTimeInSectoReverseRoll + 0.3 > Enemy.dodgeTimeRemaining)
					)
				{
					instinct_decision->defenseid = ReverseRollBSId;
				}
				//if we dont have enough time to roll, and we didnt just toggle, and we're in a neutral state; perfect block
				else if (Enemy.dodgeTimeRemaining < 0.15 && Enemy.dodgeTimeRemaining > 0 && last_subroutine_states_self[0] != ToggleEscapeId && Player.subanimation == SubanimationNeutral){
					instinct_decision->defenseid = PerfectBlockId;
				}
				//otherwise, normal roll
				else{
					instinct_decision->defenseid = StandardRollId;
				}
			}
			//if we had to toggle escape, they're probably comboing. Get out.
			if (last_subroutine_states_self[0] == ToggleEscapeId){
				instinct_decision->defenseid = StandardRollId;
			}
		}
		//windup, attack coming
		else if (AtkID == 1 || ((AtkID == 2 || AtkID == 4) && Enemy.subanimation == AttackSubanimationWindup)){
			guiPrint(LocationDetection",0:dont attack, enemy windup");
			instinct_decision->priority_decision = DelayActions;
		}
	}

	//if the enemy is close behind us, try to damage cancel their bs. TEMP: this is a bandaid and should not be permenant
	if (distanceByLine < 2 && BackstabDetection(&Enemy, &Player, distanceByLine) && !BackstabMetaOnly){
		instinct_decision->priority_decision = EnterAttackSubroutine;
		instinct_decision->attackid = GhostHitId;
	}

	//backstab checks. If AI can BS, always take it
	unsigned char BackStabStateDetected = BackstabDetection(&Player, &Enemy, distanceByLine);
	if (BackStabStateDetected){
		OverrideLowPrioritySubroutines();

		guiPrint(LocationDetection",0:backstab state %d", BackStabStateDetected);
		//in position to bs
		if (BackStabStateDetected == 2){
			instinct_decision->priority_decision = EnterAttackSubroutine;
			instinct_decision->attackid = GhostHitId;
		}
		//try and move up for bs
		else if (BackStabStateDetected == 1){
			instinct_decision->priority_decision = EnterAttackSubroutine;
			instinct_decision->attackid = MoveUpId;
		}
	}

	if (instinct_decision->priority_decision == EnemyNeutral){
		guiPrint(LocationDetection",0:not about to be hit (enemy animation type id:%d) (enemy subanimation id:%d)", Enemy.animationType_id, Enemy.subanimation);
	}
}

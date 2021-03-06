// Fill out your copyright notice in the Description page of Project Settings.

#include "BatteryNAssault.h"
#include "MechAIController.h"
#include "MechAICharacter.h"


AMechAIController::AMechAIController(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BehaviorComp = ObjectInitializer.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("BehaviorComp"));
	BlackboardComp = ObjectInitializer.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("BlackboardComp"));

}

void AMechAIController::Possess(class APawn* InPawn)
{
	Super::Possess(InPawn);

	AMechAICharacter* MechAICharacter = Cast<AMechAICharacter>(InPawn);
	if (MechAICharacter)
	{
		MyCharacter = MechAICharacter;
		if (MechAICharacter->BehaviorTree->BlackboardAsset)
		{
			BlackboardComp->InitializeBlackboard(*MechAICharacter->BehaviorTree->BlackboardAsset);
		}

		BehaviorComp->StartTree(*MechAICharacter->BehaviorTree);
	}
}


void AMechAIController::UnPossess()
{
	Super::UnPossess();

	/* Stop any behavior running as we no longer have a pawn to control */
	BehaviorComp->StopTree();
}

void AMechAIController::SetTargetEnemy(APawn* NewTarget)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsObject(TargetEnemyKeyName, NewTarget);
	}
}

void AMechAIController::SetNextWaypoint(AActor* NextWaypoint)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsObject(WaypointTarget, NextWaypoint);
	}
}

AActor* AMechAIController::GetCurrentWaypoint()
{
	if (BlackboardComp)
	{
		return Cast<AActor>(BlackboardComp->GetValueAsObject(WaypointTarget));
	}
	
	return nullptr;
}

int8 AMechAIController::GetTeamID()
{
	if (BlackboardComp)
	{
		if (MyCharacter)
		{
			return MyCharacter->TeamID;
		}
	}

	return NULL;
}


float AMechAIController::GetBatteryCharge()
{

	if (BlackboardComp)
	{
		if (MyCharacter)
		{
			return MyCharacter->GetEnergy();
		}
	}

	return NULL;
}
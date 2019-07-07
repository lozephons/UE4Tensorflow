// (C) KRAFTON INC. All right reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"

/**
 *
 */
static class ML_API LzUtil
{
public:
	template <typename... Types>
	static void LOG(const TCHAR* DebugMessage, Types... Args)
	{
		FColor Color = FColor::Red;
		ENetMode NetMode = GEngine->GetNetMode(GEngine->GetWorld());
		switch (NetMode)
		{
		case NM_DedicatedServer:
		case NM_ListenServer:
		case NM_Standalone:
			Color = FColor::Black;
			break;
		case NM_Client:
			Color = FColor::Yellow;
			break;
		}
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, Color, FString::Printf(DebugMessage, Args...));
	}

	template <typename... Types>
	static void LOG(AActor* Actor, const TCHAR* DebugMessage, Types... Args)
	{
		LOG(5.f, Actor, DebugMessage, Args...);
	}

	template <typename... Types>
	static void LOG(float time, AActor* Actor, const TCHAR* DebugMessage, Types... Args)
	{
		FColor Color = FColor::Red;
		ENetMode NetMode = Actor->GetNetMode();
		switch (NetMode)
		{
		case NM_DedicatedServer:
		case NM_ListenServer:
		case NM_Standalone:
			Color = FColor::Blue;
			break;
		case NM_Client:
			Color = FColor::Yellow;
			break;
		}
		GEngine->AddOnScreenDebugMessage(-1, time, Color, FString::Printf(DebugMessage, Args...));
	}
};

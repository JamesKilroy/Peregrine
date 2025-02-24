// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "LootTableSystem/Data/FL_LootTableHelpers.h"
#include "GameFramework/Actor.h"
#include "LootTableSystem/Components/AC_LootTable.h"

TArray<UAC_LootTable*> UFL_LootTableHelpers::GetLootTableComponents(AActor* Actor, bool IgnorePriority)
{
	TArray<UAC_LootTable*> LootTables;

	if(IgnorePriority)
	{
		Actor->GetComponents(LootTables);
		return LootTables;
	}

	Actor->GetComponents(LootTables);
	SortLootTablesByPriority(LootTables);
	return LootTables;
}

TArray<UAC_LootTable*> UFL_LootTableHelpers::SortLootTablesByPriority(TArray<UAC_LootTable*> ArrayToSort)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SortLootTablesByPriority)
	Algo::StableSort(ArrayToSort, [](const UAC_LootTable* Table1, const UAC_LootTable* Table2)
	{
		return Table1->Priority < Table2->Priority;
	});

	return ArrayToSort;
}

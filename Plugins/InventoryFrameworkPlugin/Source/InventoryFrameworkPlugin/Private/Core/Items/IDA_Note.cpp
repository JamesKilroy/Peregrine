// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Items/IDA_Note.h"


FText UIDA_Note::GetAssetTypeName()
{
	return FText(FText::FromString("Note"));
}

bool UIDA_Note::CanItemTypeStack()
{
	return false;
}

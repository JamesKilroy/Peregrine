// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Items/IDA_ItemAttachment.h"


bool UIDA_ItemAttachment::CanItemTypeStack()
{
	return false;
}

FText UIDA_ItemAttachment::GetAssetTypeName()
{
	return FText(FText::FromString("Item Attachment"));
}

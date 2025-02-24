// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Fragments/CF_CompatibilitySettings.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"

FCompatibilitySettingsFragment UCF_CompatibilitySettings::GetCompatibilitySettingsFragmentFromContainer(
	FS_ContainerSettings Container)
{
	FCompatibilitySettingsFragment* CompatibilitySettingsFragment = FindFragment<FCompatibilitySettingsFragment>(Container.ContainerFragments);
	return CompatibilitySettingsFragment ? *CompatibilitySettingsFragment : FCompatibilitySettingsFragment();
}

FCompatibilitySettingsFragment UCF_CompatibilitySettings::ToCompatibilitySettingsFragment(FInstancedStruct Struct)
{
	if(const FCompatibilitySettingsFragment* CompatibilitySettingsFragment = Struct.GetPtr<FCompatibilitySettingsFragment>())
	{
		return *CompatibilitySettingsFragment;
	}

	return FCompatibilitySettingsFragment();
}

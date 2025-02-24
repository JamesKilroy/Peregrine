// Fill out your copyright notice in the Description page of Project Settings.


#include "FL_CommonHelpers_IFP.h"

#include "GameplayTagsManager.h"

FGameplayTagContainer UFL_CommonHelpers_IFP::GetTagsChildren(FGameplayTag Tag, bool Recursive)
{
	if(Recursive)
	{
		return UGameplayTagsManager::Get().RequestGameplayTagChildren(Tag);
	}

	/**V: RequestGameplayTagChildren does not expose the Recursive boolean so I have to manually
	 * import the code here, and on top of that, the AddChildrenTags function is private so I also
	 * have to import that code here... JUST ADD THE BOOLEAN PARAMETER, EPIC :)))*/
	FGameplayTagContainer TagContainer;
	TSharedPtr<FGameplayTagNode> GameplayTagNode = UGameplayTagsManager::Get().FindTagNode(Tag);
	if (GameplayTagNode.IsValid())
	{
		TArray< TSharedPtr<FGameplayTagNode> >& ChildrenNodes = GameplayTagNode->GetChildTagNodes();
		for (TSharedPtr<FGameplayTagNode> ChildNode : ChildrenNodes)
		{
			if (ChildNode.IsValid())
			{
				TagContainer.AddTagFast(ChildNode->GetCompleteTag());
			}
		}
	}
	return TagContainer;
}

bool UFL_CommonHelpers_IFP::DoesTagHaveChildren(FGameplayTag Tag)
{
	if(!Tag.IsValid())
	{
		return false;
	}
	
	TSharedPtr<FGameplayTagNode> GameplayTagNode = UGameplayTagsManager::Get().FindTagNode(Tag);
	if (GameplayTagNode.IsValid())
	{
		return GameplayTagNode->GetChildTagNodes().IsValidIndex(0);
	}

	return false;
}

bool UFL_CommonHelpers_IFP::IsTagChildOfTag(FGameplayTag Tag, FGameplayTag PotentialParent, bool DirectChild)
{
	return GetTagsChildren(PotentialParent, !DirectChild).HasTagExact(Tag);
}

FGameplayTag UFL_CommonHelpers_IFP::GetParentTag(FGameplayTag Tag)
{
	return UGameplayTagsManager::Get().RequestGameplayTagDirectParent(Tag);
}

UWidget* UFL_CommonHelpers_IFP::GetFocusedWidget()
{
	TSharedPtr<SWidget> FocusedSlateWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
	if (!FocusedSlateWidget.IsValid())
	{
		return nullptr;
	}
	for (TObjectIterator<UWidget> Itr; Itr; ++Itr)
	{
		UWidget* CandidateUMGWidget = *Itr;
		if (CandidateUMGWidget->GetCachedWidget() == FocusedSlateWidget)
		{
			return CandidateUMGWidget;
		}
	}
	return nullptr;
}

void UFL_CommonHelpers_IFP::SetEnableWorldRendering(bool Enable)
{
	if(UGameViewportClient* ViewportClient = GEngine->GameViewport)
	{
		ViewportClient->bDisableWorldRendering = Enable;
	}
}

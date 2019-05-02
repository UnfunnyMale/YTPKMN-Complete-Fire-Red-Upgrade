#include "defines.h"
#include "../include/daycare.h"
#include "../include/constants/moves.h"
#include "../include/constants/species.h"
#include "../include/constants/items.h"
#include "../include/pokemon.h"
#include "../include/pokemon_storage_system.h"
#include "../include/new/catching.h"
#include "../include/random.h"

#define sHatchedEggFatherMoves ((u16*) 0x202455C)
#define sHatchedEggMotherMoves ((u16*)0x2024580)
#define sHatchedEggFinalMoves ((u16*) 0x2024564)
#define sHatchedEggLevelUpMoves ((u16*) 0x20244F8)

#define EGG_LVL_UP_MOVES_ARRAY_COUNT 50
#define EGG_MOVES_ARRAY_COUNT 50

extern u8 GetLevelUpMovesBySpecies(u16 species, u16* moves);

void BuildEggMoveset(struct Pokemon* egg, struct BoxPokemon* father, struct BoxPokemon* mother);

/*Priority: 
1. Volt Tackle
2. Mother's Egg Moves
3. Father's Egg Moves 
4. Father TM and HM Moves
5. Inherited Level-Up Moves 
6. Baby's Default Moveset.
*/

// called from GiveEggFromDaycare
void BuildEggMoveset(struct Pokemon* egg, struct BoxPokemon* father, struct BoxPokemon* mother)
{
	
	u16 eggSpecies = egg->species;
    u32 numLevelUpMoves, numEggMoves, numSharedParentMoves;
    u32 i, j;
	
	u16 sHatchedEggEggMoves[EGG_MOVES_ARRAY_COUNT] = {0};

    numSharedParentMoves = 0;
    for (i = 0; i < MAX_MON_MOVES; ++i)
    {
        sHatchedEggMotherMoves[i] = 0;
        sHatchedEggFatherMoves[i] = 0;
        sHatchedEggFinalMoves[i] = 0;
    }
    //for (i = 0; i < EGG_MOVES_ARRAY_COUNT; ++i)
    //    sHatchedEggEggMoves[i] = 0;
    for (i = 0; i < EGG_LVL_UP_MOVES_ARRAY_COUNT; ++i)
        sHatchedEggLevelUpMoves[i] = 0;

    numLevelUpMoves = GetLevelUpMovesBySpecies(eggSpecies, sHatchedEggLevelUpMoves);
    for (i = 0; i < MAX_MON_MOVES; ++i)
    {
        sHatchedEggFatherMoves[i] = GetBoxMonData(father, MON_DATA_MOVE1 + i, NULL);
        sHatchedEggMotherMoves[i] = GetBoxMonData(mother, MON_DATA_MOVE1 + i, NULL);
    }

    numEggMoves = GetEggMoves(egg, sHatchedEggEggMoves);
	
	//Shared Moves Between Parents
    for (i = 0; i < MAX_MON_MOVES; ++i)
    {
        for (j = 0; j < MAX_MON_MOVES && sHatchedEggFatherMoves[i] != MOVE_NONE; j++)
        {
            if (sHatchedEggFatherMoves[i] == sHatchedEggMotherMoves[j])
                sHatchedEggFinalMoves[numSharedParentMoves++] = sHatchedEggFatherMoves[i];
        }
    }
	
	//Try Assign Shared Moves to Baby
    for (i = 0; i < MAX_MON_MOVES; ++i)
    {
        for (j = 0; j < numLevelUpMoves && sHatchedEggFinalMoves[i] != MOVE_NONE; ++j)
        {
            if (sHatchedEggFinalMoves[i] == sHatchedEggLevelUpMoves[j])
            {
                if (GiveMoveToMon(egg, sHatchedEggFinalMoves[i]) == 0xFFFF)
                    DeleteFirstMoveAndGiveMoveToMon(egg, sHatchedEggFinalMoves[i]);
                break;
            }
        }
    }

#ifdef FATHER_PASSES_TMS
	//Father TMs
    for (i = 0; i < MAX_MON_MOVES; ++i)
    {
        if (sHatchedEggFatherMoves[i] != MOVE_NONE)
        {
            for (j = 0; j < NUM_TECHNICAL_MACHINES + NUM_HIDDEN_MACHINES; ++j)
            {
                if (sHatchedEggFatherMoves[i] == ItemIdToBattleMoveId(ITEM_TM01_FOCUS_PUNCH + j) && CanMonLearnTMHM(egg, j))
                {
                    if (GiveMoveToMon(egg, sHatchedEggFatherMoves[i]) == 0xFFFF)
                        DeleteFirstMoveAndGiveMoveToMon(egg, sHatchedEggFatherMoves[i]);
                }
            }
        }
    }
#endif

	//Father Egg Moves
    for (i = 0; i < MAX_MON_MOVES; ++i)
    {
        if (sHatchedEggFatherMoves[i] != MOVE_NONE)
        {
            for (j = 0; j < numEggMoves; ++j)
            {
                if (sHatchedEggFatherMoves[i] == sHatchedEggEggMoves[j])
                {
                    if (GiveMoveToMon(egg, sHatchedEggFatherMoves[i]) == 0xFFFF)
                        DeleteFirstMoveAndGiveMoveToMon(egg, sHatchedEggFatherMoves[i]);
                    break;
                }
            }
        }
    }

	//Mother Egg Moves
    for (i = 0; i < MAX_MON_MOVES; ++i)
    {
        if (sHatchedEggMotherMoves[i] != MOVE_NONE)
        {
            for (j = 0; j < numEggMoves; ++j)
            {
                if (sHatchedEggMotherMoves[i] == sHatchedEggEggMoves[j])
                {
                    if (GiveMoveToMon(egg, sHatchedEggMotherMoves[i]) == 0xFFFF)
                        DeleteFirstMoveAndGiveMoveToMon(egg, sHatchedEggMotherMoves[i]);
                    break;
                }
            }
        }
    }

	//Volt Tackle
	if (eggSpecies == SPECIES_PICHU)
	{
		if (GetBoxMonData(mother, MON_DATA_HELD_ITEM, NULL) == ITEM_LIGHT_BALL 
		||  GetBoxMonData(father, MON_DATA_HELD_ITEM, NULL) == ITEM_LIGHT_BALL)
		{
			if (GiveMoveToMon(egg, MOVE_VOLTTACKLE) == 0xFFFF)
				DeleteFirstMoveAndGiveMoveToMon(egg, MOVE_VOLTTACKLE);
		}
	}
}


s32 GetSlotToInheritNature(struct DayCare* daycare)
{
	int i;
	u8 numWithEverstone = 0;
    s32 slot = -1;

	for (i = 0; i < DAYCARE_MON_COUNT; ++i)
	{
		if (GetBoxMonData(&daycare->mons[i].mon, MON_DATA_HELD_ITEM, NULL) == ITEM_EVERSTONE)
		{
			slot = i;
			++numWithEverstone;
		}
	}
	
	if (numWithEverstone < 2)
		return slot;

    return Random() % 2; //Either nature b/c both hold everstone
}

u16 DetermineEggSpeciesAndParentSlots(struct DayCare* daycare, u8* parentSlots)
{
    u16 i;
    u16 species[DAYCARE_MON_COUNT];
    u16 eggSpecies;

    // Determine which of the daycare mons is the mother and father of the egg.
    // The 0th index of the parentSlots array is considered the mother slot, and the
    // 1st index is the father slot.
    for (i = 0; i < DAYCARE_MON_COUNT; ++i)
    {
        species[i] = GetBoxMonData(&daycare->mons[i].mon, MON_DATA_SPECIES, NULL);
        if (species[i] == SPECIES_DITTO)
        {
            parentSlots[0] = i ^ 1;
            parentSlots[1] = i;
        }
        else if (GetBoxMonGender(&daycare->mons[i].mon) == MON_FEMALE)
        {
            parentSlots[0] = i;
            parentSlots[1] = i ^ 1;
        }
    }

    eggSpecies = GetEggSpecies(species[parentSlots[0]]);
	switch(eggSpecies) {
		case SPECIES_NIDORAN_F:
			if (daycare->offspringPersonality & 0x8000)
				eggSpecies = SPECIES_NIDORAN_M;
			break;
		case SPECIES_ILLUMISE:
			if (daycare->offspringPersonality & 0x8000)
				eggSpecies = SPECIES_VOLBEAT;
			break;
		case SPECIES_MANAPHY:
			eggSpecies = SPECIES_PHIONE;
			break;
		case SPECIES_ROTOM_HEAT:
		case SPECIES_ROTOM_WASH:
		case SPECIES_ROTOM_FROST:
		case SPECIES_ROTOM_FAN:
		case SPECIES_ROTOM_MOW:
			eggSpecies = SPECIES_ROTOM;
			break;
		case SPECIES_FURFROU_HEART:
		case SPECIES_FURFROU_DIAMOND:
		case SPECIES_FURFROU_STAR:
		case SPECIES_FURFROU_PHAROAH:
		case SPECIES_FURFROU_KABUKI:
		case SPECIES_FURFROU_LA_REINE:
		case SPECIES_FURFROU_MATRON:
		case SPECIES_FURFROU_DANDY:
		case SPECIES_FURFROU_DEBUTANTE:
			eggSpecies = SPECIES_FURFROU;
	}

    // Make Ditto the "mother" slot if the other daycare mon is male.
    if (species[parentSlots[1]] == SPECIES_DITTO && GetBoxMonGender(&daycare->mons[parentSlots[0]].mon) != MON_FEMALE)
    {
        u8 temp = parentSlots[1];
        parentSlots[1] = parentSlots[0];
        parentSlots[0] = temp;
    }

    return eggSpecies;
}



enum {
	HpIv = 0,
	AtkIv,
	DefIv,
	SpdIv,
	SpAtkIv,
	SpDefIv,
};


u8 CheckPowerItem(u16 item) {
	//u16 item = GetBoxMonData(&daycare->mons[parent].mon, MON_DATA_HELD_ITEM, NULL);
	
	// use hold effect / hold parameter instead?
	switch (item)
	{
		case ITEM_POWER_BRACER:
			return HpIv;
		case ITEM_POWER_BELT:
			return AtkIv;
		case ITEM_POWER_LENS:
			return DefIv;
		case ITEM_POWER_BAND:
			return SpdIv;
		case ITEM_POWER_ANKLET:
			return SpAtkIv;
		case ITEM_POWER_WEIGHT:
			return SpDefIv;
		default:
			return 0xFF;
	}
};




void InheritIVs(struct Pokemon *egg, struct DayCare *daycare) {
    u8 i, numIVs, iv;
	u16 items[2];
	
	items[0] = GetBoxMonData(&daycare->mons[0].mon, MON_DATA_HELD_ITEM, NULL);
	items[1] = GetBoxMonData(&daycare->mons[1].mon, MON_DATA_HELD_ITEM, NULL);
		
	// get number of IVs to inherit from either parent
	if (items[0] == ITEM_DESTINY_KNOT || items[1] == ITEM_DESTINY_KNOT)
		numIVs = 5;
	else
		numIVs = 3;
	
    u8 selectedIvs[numIVs];
    //u8 availableIVs[NUM_STATS];
    u8 whichParent[numIVs];
		
	// initiate first 1 or 2 IV slots with power items from either or both parents
	u8 initVal = 0;
	u8 powerResult = 0xFF;
	for (i = 0; i < DAYCARE_MON_COUNT; ++i)
	{
		powerResult = CheckPowerItem(items[i]);	// check if parent i has a power item
		if (powerResult == 0xFF)
			continue;	// parent does not have a power item
		
		// parent has a power item, save index to IV slot
		if (items[0] == items[1])
		{
			// both parents have same power item, choose parent at random
			whichParent[i] = Random() % 2;
			selectedIvs[i] = powerResult;
			initVal++;
			break;
		}
		else
		{
			whichParent[i] = i;
			selectedIvs[i] = powerResult;
			initVal++;
		}
	}
	
	// randomize the remaining IV indices with unique values
	bool8 unique;
	for (i = initVal; i < ARRAY_COUNT(selectedIvs); ++i)
	{
		iv = Random() % NUM_STATS;
		
		// check if index is unique
		unique = TRUE;
		u8 j = 0;
		while (unique == TRUE)
		{
			if (iv == selectedIvs[j])
				unique = FALSE;
			
			if (j == NUM_STATS)
			{
				selectedIvs[i] = iv;
				break;
			}
			j++;
		}
		whichParent[i] = Random() % 2;
	}
	
	
	
/*			original routine
    // Initialize a list of IV indices.
    for (i = 0; i < NUM_STATS; i++)
    {
        availableIVs[i] = i;
    }

    // Select the num IVs that will be inherited.
    for (i = 0; i < ARRAY_COUNT(selectedIvs); i++)
    {
        // Randomly pick an IV from the available list.
        selectedIvs[i] = availableIVs[Random() % (NUM_STATS - i)];

        // Remove the selected IV index from the available IV indices.
        RemoveIVIndexFromList(availableIVs, i);
    }

    // Determine which parent each of the selected IVs should inherit from.
    for (i = 0; i < ARRAY_COUNT(selectedIvs); i++)
    {
        whichParent[i] = Random() % 2;
    }
	
*/

    // Set each of inherited IVs on the egg mon.
    for (i = 0; i < ARRAY_COUNT(selectedIvs); ++i)
    {
        switch (selectedIvs[i])
        {
            case HpIv:
                iv = GetBoxMonData(&daycare->mons[whichParent[i]].mon, MON_DATA_HP_IV, NULL);
                SetMonData(egg, MON_DATA_HP_IV, &iv);
                break;
            case AtkIv:
                iv = GetBoxMonData(&daycare->mons[whichParent[i]].mon, MON_DATA_ATK_IV, NULL);
                SetMonData(egg, MON_DATA_ATK_IV, &iv);
                break;
            case DefIv:
                iv = GetBoxMonData(&daycare->mons[whichParent[i]].mon, MON_DATA_DEF_IV, NULL);
                SetMonData(egg, MON_DATA_DEF_IV, &iv);
                break;
            case SpdIv:
                iv = GetBoxMonData(&daycare->mons[whichParent[i]].mon, MON_DATA_SPEED_IV, NULL);
                SetMonData(egg, MON_DATA_SPEED_IV, &iv);
                break;
            case SpAtkIv:
                iv = GetBoxMonData(&daycare->mons[whichParent[i]].mon, MON_DATA_SPATK_IV, NULL);
                SetMonData(egg, MON_DATA_SPATK_IV, &iv);
                break;
            case SpDefIv:
                iv = GetBoxMonData(&daycare->mons[whichParent[i]].mon, MON_DATA_SPDEF_IV, NULL);
                SetMonData(egg, MON_DATA_SPDEF_IV, &iv);
                break;
        }
    }
};


void AlterEggSpeciesWithIncenseItem(u16 *species, struct DayCare *daycare) {
	u16 motherItem = GetBoxMonData(&daycare->mons[0].mon, MON_DATA_HELD_ITEM, NULL);
	u16 fatherItem = GetBoxMonData(&daycare->mons[1].mon, MON_DATA_HELD_ITEM, NULL);   
	
	// if neither parent holding incense, force 2nd evo species
	switch (*species)
	{
		case SPECIES_WYNAUT:
			if (motherItem != ITEM_LAX_INCENSE && fatherItem != ITEM_LAX_INCENSE)
				*species = SPECIES_WOBBUFFET;
			break;
		case SPECIES_AZURILL:
			if (motherItem != ITEM_SEA_INCENSE && fatherItem != ITEM_SEA_INCENSE)
				*species = SPECIES_MARILL;
			break;
		case SPECIES_MUNCHLAX:
			if (motherItem != ITEM_FULL_INCENSE && fatherItem != ITEM_FULL_INCENSE)
				*species = SPECIES_SNORLAX;
			break;
		case SPECIES_MIME_JR:
			if (motherItem != ITEM_ODD_INCENSE && fatherItem != ITEM_ODD_INCENSE)
				*species = SPECIES_MR_MIME;
			break;
		case SPECIES_CHINGLING:
			if (motherItem != ITEM_PURE_INCENSE && fatherItem != ITEM_PURE_INCENSE)
				*species = SPECIES_CHIMECHO;
			break;
		case SPECIES_BONSLY:
			if (motherItem != ITEM_ROCK_INCENSE && fatherItem != ITEM_ROCK_INCENSE)
				*species = SPECIES_SUDOWOODO;
			break;
		case SPECIES_BUDEW:
			if (motherItem != ITEM_ROSE_INCENSE && fatherItem != ITEM_ROSE_INCENSE)
				*species = SPECIES_ROSELIA;
			break;
		case SPECIES_MANTYKE:
			if (motherItem != ITEM_WAVE_INCENSE && fatherItem != ITEM_WAVE_INCENSE)
				*species = SPECIES_MANTINE;
			break;
		case SPECIES_HAPPINY:
			if (motherItem != ITEM_LUCK_INCENSE && fatherItem != ITEM_LUCK_INCENSE)
				*species = SPECIES_CHANSEY;
			break;
		default:
			break;
	}
};

/* ORIGINAL	
void AlterEggSpeciesWithIncenseItem(u16 *species, struct DayCare *daycare) {
	u16 motherItem, fatherItem;
    if (*species == SPECIES_WYNAUT || *species == SPECIES_AZURILL)
    {
        motherItem = GetBoxMonData(&daycare->mons[0].mon, MON_DATA_HELD_ITEM);
        fatherItem = GetBoxMonData(&daycare->mons[1].mon, MON_DATA_HELD_ITEM);
        if (*species == SPECIES_WYNAUT && motherItem != ITEM_LAX_INCENSE && fatherItem != ITEM_LAX_INCENSE)
        {
            *species = SPECIES_WOBBUFFET;
        }

        if (*species == SPECIES_AZURILL && motherItem != ITEM_SEA_INCENSE && fatherItem != ITEM_SEA_INCENSE)
        {
            *species = SPECIES_MARILL;
        }
    }
};
*/


void InheritPokeBall(struct Pokemon *egg, struct DayCare *daycare) {
	u16 motherSpecies = GetBoxMonData(&daycare->mons[0].mon, MON_DATA_SPECIES, NULL);
	u16 fatherSpecies = GetBoxMonData(&daycare->mons[1].mon, MON_DATA_SPECIES, NULL);
	
	u8 parent = 0;	// mother default
	// gen 7 same species check
	if (motherSpecies == fatherSpecies)
		parent = Random() % 2;	// same parent species -> pokeball inherited randomly
	
	// get poke ball ID
	u8 parentBall = GetBoxMonData(&daycare->mons[parent].mon, MON_DATA_POKEBALL, NULL);
	
	// master ball and cherish ball become poke ball
	#ifndef INHERIT_MASTER_CHERISH_BALL
	if (parentBall == BALL_TYPE_MASTER_BALL || parentBall == BALL_TYPE_CHERISH_BALL)
		parentBall = BALL_TYPE_POKE_BALL;
	#endif
	
	SetMonData(egg, MON_DATA_POKEBALL, &parentBall);	
};


/*
void CreateMon(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId)
void SetInitialEggData(struct Pokemon *mon, u16 species, struct DayCare *daycare) {
    u32 personality;
    u16 ball;
    u8 metLevel;
    u8 language;

    personality = daycare->offspringPersonality;
    
	CreateMon(mon, species, EGG_HATCH_LEVEL, 0x20, TRUE, personality, FALSE, 0);
    
	metLevel = 0;
    ball = ITEM_POKE_BALL;
    language = LANGUAGE_JAPANESE;
    SetMonData(mon, MON_DATA_POKEBALL, &ball);
    SetMonData(mon, MON_DATA_NICKNAME, sJapaneseEggNickname);
    SetMonData(mon, MON_DATA_FRIENDSHIP, &gBaseStats[species].eggCycles);
    SetMonData(mon, MON_DATA_MET_LEVEL, &metLevel);
    SetMonData(mon, MON_DATA_LANGUAGE, &language);
}
*/


// Decide features to inherit
void GiveEggFromDaycare(struct DayCare *daycare) {
    struct Pokemon egg;
    u16 species;
    u8 parentSlots[2]; // 0th index is "mother" daycare slot, 1st is "father"
    bool8 isEgg;

    species = DetermineEggSpeciesAndParentSlots(daycare, parentSlots);
    AlterEggSpeciesWithIncenseItem(&species, daycare);
    SetInitialEggData(&egg, species, daycare);	// sets base data (ball, met level, lang, etc)
    InheritIVs(&egg, daycare);	// destiny knot check
	InheritPokeBall(&egg, daycare);
    BuildEggMoveset(&egg, &daycare->mons[parentSlots[1]].mon, &daycare->mons[parentSlots[0]].mon);

	// handled in BuildEggMoveset
    //if (species == SPECIES_PICHU)
    //    GiveVoltTackleIfLightBall(&egg, daycare);

    isEgg = TRUE;
    SetMonData(&egg, MON_DATA_IS_EGG, &isEgg);
    gPlayerParty[PARTY_SIZE - 1] = egg;
    CompactPartySlots();
    CalculatePlayerPartyCount();
    RemoveEggFromDayCare(daycare);
};



void TriggerPendingDaycareEgg(struct DayCare *daycare) {
    s32 natureSlot;
    s32 natureTries = 0;

    SeedRng2(gMain.vblankCounter2);
	
    natureSlot = GetSlotToInheritNature(daycare);	// updated nature slot check

    if (natureSlot < 0)
    {
        daycare->offspringPersonality = (Random2() << 0x10) | ((Random() % 0xfffe) + 1);
    }
    else
    {
        u8 wantedNature = GetNatureFromPersonality(GetBoxMonData(&daycare->mons[natureSlot].mon, MON_DATA_PERSONALITY, NULL));
        u32 personality;

        do
        {
            personality = (Random2() << 0x10) | (Random());
            if (wantedNature == GetNatureFromPersonality(personality) && personality != 0)
                break; // we found a personality with the same nature

            natureTries++;
        } while (natureTries <= 2400);

        daycare->offspringPersonality = personality;
    }

    FlagSet(FLAG_PENDING_DAYCARE_EGG);
};


/*
void CreateBoxMon(struct BoxPokemon *boxMon, u16 species, u8 level, u8 fixedIV, u8 hasFixedPersonality, u32 fixedPersonality, u8 otIdType, u32 fixedOtId)
{
    u8 speciesName[POKEMON_NAME_LENGTH + 1];
    u32 personality;
    u32 value;
    u16 checksum;

    ZeroBoxMonData(boxMon);

    if (hasFixedPersonality)
        personality = fixedPersonality;
    else
        personality = Random32();

    SetBoxMonData(boxMon, MON_DATA_PERSONALITY, &personality);

    //Determine original trainer ID
    
    // Shiny Charm
    
    if (otIdType == OT_ID_RANDOM_NO_SHINY) //Pokemon cannot be shiny
    {
        u32 shinyValue;
        do
        {
            value = Random32();
            shinyValue = HIHALF(value) ^ LOHALF(value) ^ HIHALF(personality) ^ LOHALF(personality);
        } while (shinyValue < 8);
    }
    else if (otIdType == OT_ID_PRESET) //Pokemon has a preset OT ID
    {
        value = fixedOtId;
    }
    else //Player is the OT
    {
        value = gSaveBlock2Ptr->playerTrainerId[0]
              | (gSaveBlock2Ptr->playerTrainerId[1] << 8)
              | (gSaveBlock2Ptr->playerTrainerId[2] << 16)
              | (gSaveBlock2Ptr->playerTrainerId[3] << 24);
    }

    SetBoxMonData(boxMon, MON_DATA_OT_ID, &value);

    checksum = CalculateBoxMonChecksum(boxMon);
    SetBoxMonData(boxMon, MON_DATA_CHECKSUM, &checksum);
    EncryptBoxMon(boxMon);
    GetSpeciesName(speciesName, species);
    SetBoxMonData(boxMon, MON_DATA_NICKNAME, speciesName);
    SetBoxMonData(boxMon, MON_DATA_LANGUAGE, &gGameLanguage);
    SetBoxMonData(boxMon, MON_DATA_OT_NAME, gSaveBlock2Ptr->playerName);
    SetBoxMonData(boxMon, MON_DATA_SPECIES, &species);
    SetBoxMonData(boxMon, MON_DATA_EXP, &gExperienceTables[gBaseStats[species].growthRate][level]);
    SetBoxMonData(boxMon, MON_DATA_FRIENDSHIP, &gBaseStats[species].friendship);
    value = GetCurrentRegionMapSectionId();
    SetBoxMonData(boxMon, MON_DATA_MET_LOCATION, &value);
    SetBoxMonData(boxMon, MON_DATA_MET_LEVEL, &level);
    SetBoxMonData(boxMon, MON_DATA_MET_GAME, &gGameVersion);
    value = ITEM_POKE_BALL;
    SetBoxMonData(boxMon, MON_DATA_POKEBALL, &value);
    SetBoxMonData(boxMon, MON_DATA_OT_GENDER, &gSaveBlock2Ptr->playerGender);

    if (fixedIV < 32)
    {
        SetBoxMonData(boxMon, MON_DATA_HP_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_ATK_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_DEF_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &fixedIV);
        SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &fixedIV);
    }
    else
    {
        u32 iv;
        value = Random();

        iv = value & 0x1F;
        SetBoxMonData(boxMon, MON_DATA_HP_IV, &iv);
        iv = (value & 0x3E0) >> 5;
        SetBoxMonData(boxMon, MON_DATA_ATK_IV, &iv);
        iv = (value & 0x7C00) >> 10;
        SetBoxMonData(boxMon, MON_DATA_DEF_IV, &iv);

        value = Random();

        iv = value & 0x1F;
        SetBoxMonData(boxMon, MON_DATA_SPEED_IV, &iv);
        iv = (value & 0x3E0) >> 5;
        SetBoxMonData(boxMon, MON_DATA_SPATK_IV, &iv);
        iv = (value & 0x7C00) >> 10;
        SetBoxMonData(boxMon, MON_DATA_SPDEF_IV, &iv);
    }

    if (gBaseStats[species].ability2)
    {
        value = personality & 1;
        SetBoxMonData(boxMon, MON_DATA_ALT_ABILITY, &value);
    }

    GiveBoxMonInitialMoveset(boxMon);
}
*/
#include "main.h"
#include "board_id.h"

#define STM32_UID_WORD0_ADDR ((uint32_t)0x1FFFF7ACU)
#define STM32_UID_WORD1_ADDR ((uint32_t)0x1FFFF7B0U)
#define STM32_UID_WORD2_ADDR ((uint32_t)0x1FFFF7B4U)

static const BoardID_t kKnownBoardIDs[BOARD_NUM] =
{
    {BOARD_UID_0},
    {BOARD_UID_1},
    {BOARD_UID_2},
    {BOARD_UID_3},
};

static bool BoardID_Equals(const BoardID_t *lhs, const BoardID_t *rhs)
{
    if ((lhs == 0) || (rhs == 0))
    {
        return false;
    }

    for (uint32_t i = 0; i < BOARD_ID_WORD_COUNT; ++i)
    {
        if (lhs->words[i] != rhs->words[i])
        {
            return false;
        }
    }

    return true;
}

bool BoardID_Read(BoardID_t *board_id)
{
    if (board_id == 0)
    {
        return false;
    }

    board_id->words[0] = *(const uint32_t *)STM32_UID_WORD0_ADDR;
    board_id->words[1] = *(const uint32_t *)STM32_UID_WORD1_ADDR;
    board_id->words[2] = *(const uint32_t *)STM32_UID_WORD2_ADDR;

    return true;
}

int8_t BoardID_Detect(void)
{
    BoardID_t current_board_id;

    if (!BoardID_Read(&current_board_id))
    {
        return BOARD_ID_INVALID;
    }

    for (int8_t i = 0; i < BOARD_NUM; ++i)
    {
        if (BoardID_Equals(&current_board_id, &kKnownBoardIDs[i]))
        {
            return i;
        }
    }

    return BOARD_ID_INVALID;
}

bool BoardID_IsValid(int8_t board_id)
{
    return (board_id >= 0) && (board_id < BOARD_NUM);
}

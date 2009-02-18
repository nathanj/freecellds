#include <list>
#include <vector>
#include <cstdlib>

#include <stdio.h>
#include <string.h>

#include <nds.h>

#include "background.h"
#include "background2.h"
#include "tiles.h"

#define DEBUG

// Windows rand() function. Used to get the same deals as Windows Freecell.
long holdrand;
void win_srand (long seed)
{
	holdrand = seed;
}
int win_rand (void)
{
	return(((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}


SpriteEntry spriteEntry[128];
SpriteRotation *spriteRotation = (SpriteRotation*)spriteEntry;

struct TouchHelper {
	bool held, down, up;
	touchPosition touch;
	u16 prevx, prevy;
};

enum Color { BLACK, RED };
enum Suit { CLUBS, DIAMONDS, HEARTS, SPADES };
enum Value { ACE = 1, DEUCE, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
	TEN, JACK, QUEEN, KING };

#define COLOR(card) ((SUIT(card) == HEARTS || SUIT(card) == DIAMONDS) ? RED : BLACK)
#define SUIT(card) (card%4)
#define VALUE(card) ((card/4)+1)

class Card {
public:
	int card;

	Card()
	{
	}

	void Initialize(int card, int x, int y)
	{
		this->card = card;
	}

	inline int color() const
	{
		return COLOR(card);
	}

	inline int suit() const
	{
		return SUIT(card);
	}

	inline int value() const
	{
		return VALUE(card);
	}
};

class FreeCellGame {
public:
	/// The eight stacks in the playing field.
	std::vector<Card*> stack[8];
	/// The four free cells.
	Card *freecells[4];
	/// The four finished stacks.
	std::vector<Card*> finish[4];
	/// The stack that is currently being moved by the player.
	std::vector<Card*> moving;
	/// The deck of cards.
	Card cards[52];
	/// The shuffled pointers of cards.
	Card *shuffled[52];

	int movingx, movingy;
	int stack_moving_from;

	FreeCellGame()
	{
	}

	void Initialize()
	{
		movingx = movingy = 0;

		for (int i = 0; i < 8; i++)
			stack[i].reserve(21);

		for (int i = 0; i < 4; i++)
			finish[i].reserve(13);

		moving.reserve(13);

		for (int i = 0; i < 4; i++)
			freecells[i] = NULL;

		for (int i = 0; i < 52; i++)
			cards[i].Initialize(i, 0, 0);

		Deal();

		SetAllCardPos();
	}

	void Deal()
	{
		for (int i = 0; i < 52; i++)
			shuffled[i] = &cards[i];

		for (int i = 0; i < 8; i++)
			stack[i].clear();
		printf("%ld\n", time(NULL));

		win_srand(164);
		int wLeft = 52;
		for (int i = 0; i < 52; i++)
		{
			int j = win_rand() % wLeft;
			stack[i%8].push_back(shuffled[j]);
			shuffled[j] = shuffled[--wLeft];
		}
	}

	/// Moves the bottom n cards from the first stack to the second.
	void MoveStack(int n, int s1, int s2)
	{
		for (std::vector<Card*>::iterator it = stack[s1].end() - n;
				it != stack[s1].end(); ++it)
			stack[s2].push_back(*it);
		stack[s1].erase(stack[s1].end() - n, stack[s1].end());
	}

	/// Sets the x and y position for all cards depending on what stack they
	/// are in.
	void SetAllCardPos()
	{
		int pos = 0;

		for (int j = moving.size() - 1; j >= 0; j--)
		{
			int suit = moving[j]->suit();
			int value = moving[j]->value() - 1;
			int index = (value / 2) * 128 + (value % 2) * 16 + suit * 4;

			spriteEntry[pos].attribute[0] = ATTR0_COLOR_16 | ATTR0_NORMAL | ATTR0_SQUARE;
			spriteEntry[pos].attribute[1] = ATTR1_SIZE_32;
			spriteEntry[pos].attribute[2] = ATTR2_PRIORITY(3) | index;
			spriteEntry[pos].x = movingx - 8;
			spriteEntry[pos].y = movingy - 4 + j*11;
			pos++;
		}

		for (int i = 0; i < 8; i++)
		{
			for (int j = stack[i].size() - 1; j >= 0; j--)
			{
				int suit = stack[i][j]->suit();
				int value = stack[i][j]->value() - 1;
				int index = (value / 2) * 128 + (value % 2) * 16 + suit * 4;

				spriteEntry[pos].attribute[0] = ATTR0_COLOR_16 | ATTR0_NORMAL | ATTR0_SQUARE;
				spriteEntry[pos].attribute[1] = ATTR1_SIZE_32;
				spriteEntry[pos].attribute[2] = ATTR2_PRIORITY(3) | index;
				spriteEntry[pos].x = i*32;
				spriteEntry[pos].y = j*11 + 48;

				pos++;
			}
		}

		for (int i = 0; i < 4; i++)
		{
			if (freecells[i] != NULL)
			{
				int suit = freecells[i]->suit();
				int value = freecells[i]->value() - 1;
				int index = (value / 2) * 128 + (value % 2) * 16 + suit * 4;

				spriteEntry[pos].attribute[0] = ATTR0_COLOR_16 | ATTR0_NORMAL | ATTR0_SQUARE;
				spriteEntry[pos].attribute[1] = ATTR1_SIZE_32;
				spriteEntry[pos].attribute[2] = ATTR2_PRIORITY(3) | index;
				spriteEntry[pos].x = i*32;
				spriteEntry[pos].y = 12;

				pos++;
			}
		}

		for (int i = 0; i < 4; i++)
		{
			if (!finish[i].empty())
			{
				int suit = finish[i][finish[i].size()-1]->suit();
				int value = finish[i][finish[i].size()-1]->value() - 1;
				int index = (value / 2) * 128 + (value % 2) * 16 + suit * 4;

				spriteEntry[pos].attribute[0] = ATTR0_COLOR_16 | ATTR0_NORMAL | ATTR0_SQUARE;
				spriteEntry[pos].attribute[1] = ATTR1_SIZE_32;
				spriteEntry[pos].attribute[2] = ATTR2_PRIORITY(3) | index;
				spriteEntry[pos].x = (4+i)*32;
				spriteEntry[pos].y = 12;

				pos++;
			}
		}

		while (pos < 128)
			spriteEntry[pos++].attribute[0] = ATTR0_DISABLED;
	}

	/// Returns true if card1 can be moved under card2.
	bool IsCardUnderCard(const Card *card1, const Card *card2)
	{
		return (card1->color() != card2->color() &&
				card1->value() == card2->value() - 1);
	}

	/// Returns true if card1 can be moved over card2 on the finished stacks.
	bool IsCardOverCardFinish(const Card *card1, const Card *card2)
	{
		return (card1->suit() == card2->suit() &&
				card1->value() == card2->value() + 1);
	}

	/// Returns true if the card in position n of stack st can be moved.
	bool CanSelectCard(int st, int n)
	{
		int num_freecells = 0;
		for (int i = 0; i < 4; i++)
			if (freecells[i] == NULL)
				num_freecells++;

		int size = stack[st].size();

		if (size - n > num_freecells + 1)
			return false;

		while (n < size - 1)
		{
			if (!IsCardUnderCard(stack[st][n+1], stack[st][n]))
				return false;
			n++;
		}

		return true;
	}

	void SelectCards(const TouchHelper &touch)
	{
		int px = touch.touch.px;
		int py = touch.touch.py;

		unsigned int selx = px / 32;
		unsigned int sely = (py - 48) / 11;

		if (selx > 7)
			selx = 7;

		if (py < 40)
		{
			// Handle selecting from freecells.
			if (selx < 4)
			{
				if (freecells[selx] != NULL)
				{
					moving.push_back(freecells[selx]);
					stack_moving_from = selx + 8;
					freecells[selx] = NULL;

					movingx = px;
					movingy = py;
				}
			}
		}
		else
		{
			if (!CanSelectCard(selx, sely))
				return;

			if (stack[selx].size() > sely)
			{
				for (std::vector<Card*>::iterator it = stack[selx].begin() + sely;
						it != stack[selx].end(); ++it)
				{
					moving.push_back(*it);
				}
				stack[selx].erase(stack[selx].begin() + sely, stack[selx].end());
			}

			movingx = px;
			movingy = py;

			stack_moving_from = selx;
		}
	}

	void RevertMove()
	{
		if (stack_moving_from < 8)
		{
			for (std::vector<Card*>::iterator it = moving.begin();
					it != moving.end(); ++it)
			{
				stack[stack_moving_from].push_back(*it);
			}
		}
		else
		{
			freecells[stack_moving_from - 8] = moving[0];
		}

		moving.clear();
	}

	void MoveCards(const TouchHelper &touch)
	{
		int px = touch.prevx + 16;
		int py = touch.prevy;

		unsigned int selx = (px - 8) / 32;

		if (selx > 7)
			selx = 7;

		if (moving.empty())
			return;

		if (py < 40)
		{
			// Handle moving to freecells.
			if (selx < 4)
			{
				if (freecells[selx] == NULL && moving.size() == 1)
				{
					freecells[selx] = moving[0];
				}
				else
					RevertMove();
				moving.clear();
			}
			// Handle moving to the finished stacks.
			else
			{
				selx -= 4;
				if (moving.size() != 1)
				{
					RevertMove();
				}
				else
				{
					if (finish[selx].empty() && moving[0]->value() == ACE)
					{
						finish[selx].push_back(moving[0]);
						moving.clear();
					}
					else if (!finish[selx].empty() &&
							IsCardOverCardFinish(moving[0], finish[selx][finish[selx].size()-1]))
					{
						finish[selx].push_back(moving[0]);
						moving.clear();
					}
					else
					{
						RevertMove();
					}
				}
			}
		}
		else
		{
			// Handle moving to stack.
			if (!stack[selx].empty() &&
					!IsCardUnderCard(moving[0], stack[selx][stack[selx].size()-1]))
			{
				RevertMove();
			}
			else
			{
				for (std::vector<Card*>::iterator it = moving.begin();
						it != moving.end(); ++it)
				{
					stack[selx].push_back(*it);
				}
				moving.clear();
			}
		}
	}

	void HandleTouch(const TouchHelper &touch)
	{
		if (touch.down)
			SelectCards(touch);
		else if (touch.held)
		{
			movingx = touch.touch.px;
			movingy = touch.touch.py;
		}
		else if (touch.up)
			MoveCards(touch);
	}

	void Update(const TouchHelper &touch)
	{
		HandleTouch(touch);
		SetAllCardPos();
	}
};

inline void dmaFlushCopy(void *src, void *dst, size_t size)
{
	//DC_FlushRange(src, size);
	DC_FlushAll();
	dmaCopy(src, dst, size);
}


void updateOAM() {
    //DC_FlushAll();
	memcpy(OAM, spriteEntry, 128*sizeof(SpriteEntry));
    //dmaCopyHalfWords(3, spriteEntry, OAM, 128 * sizeof(SpriteEntry));
}

int main(void) {
	int i;
	TouchHelper touch;

	powerOn(POWER_ALL_2D);
	lcdMainOnBottom();

#ifdef DEBUG
	consoleDemoInit();
#endif

	// Set the video ram banks.
    vramSetMainBanks(VRAM_A_MAIN_BG_0x06000000,
                     VRAM_B_MAIN_SPRITE_0x06400000,
                     VRAM_C_SUB_BG_0x06200000,
                     VRAM_D_LCD);
 
	// Set the graphic modes.
    videoSetMode(MODE_5_2D |
                 DISPLAY_BG3_ACTIVE |
                 DISPLAY_SPR_ACTIVE |
                 DISPLAY_SPR_2D);
#ifndef DEBUG
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
#endif

	// Initialize the background.
	bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	REG_BG3CNT |= BG_PRIORITY_3;
#ifndef DEBUG
	bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	REG_BG3CNT_SUB |= BG_PRIORITY_3;
#endif

	// Initialize the sprites to all be off.
	for (i = 0; i < 128; i++) {
		spriteEntry[i].attribute[0] = ATTR0_DISABLED;
		spriteEntry[i].attribute[1] = 0;
		spriteEntry[i].attribute[2] = 0;
	}
	for (i = 0; i < 32; i++) {
		spriteRotation[i].hdx = 256;
		spriteRotation[i].hdy = 0;
		spriteRotation[i].vdx = 0;
		spriteRotation[i].vdy = 256;
	}

	// Copy the graphics to video ram.
	DC_FlushAll();
	dmaCopy(tilesPal, SPRITE_PALETTE, sizeof(tilesPal));
	dmaCopy(tilesTiles, SPRITE_GFX, sizeof(tilesTiles));
	decompress(backgroundBitmap, BG_GFX, LZ77Vram);
#ifndef DEBUG
	decompress(background2Bitmap, BG_GFX_SUB, LZ77Vram);
#endif

	// Start the game.
	FreeCellGame game;
	game.Initialize();

	while (1) {
		scanKeys();
		touch.prevx = touch.touch.px;
		touch.prevy = touch.touch.py;
		touchRead(&touch.touch);
		touch.held = keysHeld() & KEY_TOUCH;
		touch.down = keysDown() & KEY_TOUCH;
		touch.up   = keysUp() & KEY_TOUCH;

		game.Update(touch);

		swiWaitForVBlank();
		updateOAM();
	}
 
	return 0;
}


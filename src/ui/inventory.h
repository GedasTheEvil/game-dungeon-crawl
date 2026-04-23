#ifndef InventoryH
#define InventoryH

#include "../graphics/font.h"
#include "../entities/item.h"
#include "fstream"
#include "../core/timer.h"

namespace ItemType {
constexpr int MELEE_WEAPON = 1;
constexpr int RANGED_WEAPON = 2;
constexpr int POTION = 3;
} // namespace ItemType

namespace WeaponId {
constexpr int CLUB = 0;
constexpr int SWORD = 1;
constexpr int SPEAR = 2;
constexpr int BOW = 0;
} // namespace WeaponId

namespace PotionId {
constexpr int SMALL_HEALTH = 0;
constexpr int LARGE_HEALTH = 1;
constexpr int STRENGTH = 2;
constexpr int ARMOR = 3;
constexpr int LIFE = 4;
constexpr int COUNT = 5;
} // namespace PotionId

struct InvItem {
	int id = 0;
	int count = 0;
	float realScale = 0.0f;
};

struct eq {
	int type = 0;
	int id = 0;
	int count = 0;
};

class inventory {
  private:
	InvItem mw[3]; // melee weapon
	InvItem rw;	   // ranged weapon
	eq equipped;
	eq view;
	InvItem potions[PotionId::COUNT];
	Font Impact, Scribe, small;
	timer* inv_ani;

	void SelectItem(item* itemPtr, int type, int id, int count);
	void DrawItemSlot(int x1, int y1, int x2, int y2);
	void DrawItemModel(item* itemPtr, float posX, float posY, float scale, float& realScale, bool rotate);
	void DrawWeaponInfo(int weaponId);
	void DrawPotionInfo(int potionId);

  public:
	bool show; // if true, show inventory
	inventory();
	~inventory();
	void UsePotion();
	void GetItem(int type, int ID);
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	item* Equipped();
	void Dump(std::ofstream& f);
	void LoadDump(std::ifstream& f);
};

#endif

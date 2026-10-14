#include "pch.h"
#include "Inventory.h"
#include "EngineState.h"

namespace Framework {

    Inventory& Inventory::Instance()
    {
        static Inventory g;
        return g;
    }

    // =========================================================
    // HOTBAR BASIC OPERATIONS
    // =========================================================

    int Inventory::AddFirstEmpty(int itemId, GLuint iconTex, int count)
    {
        const int hotSlots = GetHotbarSlotCount();
        for (int i = 0; i < hotSlots; ++i)
        {
            if (m_hotbar[i].itemId == -1)
            {
                m_hotbar[i] = { itemId, iconTex, count };
                return i;
            }
        }
        return -1;
    }

    void Inventory::SetHotbarSlot(int idx, int itemId, GLuint iconTex, int count)
    {
        if (idx < 0 || idx >= GetHotbarSlotCount()) return;
        m_hotbar[idx] = { itemId, iconTex, count };
    }

    void Inventory::ClearHotbarSlot(int idx)
    {
        if (idx < 0 || idx >= GetHotbarSlotCount()) return;
        m_hotbar[idx] = { -1, 0, 0 };
    }

    void Inventory::SetSelectedIndex(int idx)
    {
        int maxSlots = GetHotbarSlotCount();
        if (maxSlots <= 0) maxSlots = 1;
        if (idx < 0) idx = 0;
        if (idx >= maxSlots) idx = maxSlots - 1;
        m_selectedIndex = idx;
    }

    void Inventory::ChangeSelected(int delta)
    {
        delta = -delta;
        int maxSlots = GetHotbarSlotCount();
        if (maxSlots <= 0) maxSlots = 1;
        int i = (m_selectedIndex + delta) % maxSlots;
        if (i < 0) i += maxSlots;
        m_selectedIndex = i;

        // Only play scroll sound during active gameplay
        if (g_AudioSystem && engineState.Is(EngineMode::Play) && !engineState.IsAnyUIOpen()) {
            g_AudioSystem->PlayAudio("event:/InventorySelect");
        }
    }

    int Inventory::GetSelectedItemId() const
    {
        const int maxSlots = GetHotbarSlotCount();
        if (m_selectedIndex < 0 || m_selectedIndex >= maxSlots) return -1;
        return m_hotbar[m_selectedIndex].itemId;
    }

    // =========================================================
    // HOTBAR UPGRADE
    // =========================================================

    void Inventory::SetHotbarLevel(int level)
    {
        if (level < 0) level = 0;
        mHotbarLevel = level;
        int maxSlots = GetHotbarSlotCount();
        if (m_selectedIndex >= maxSlots) m_selectedIndex = maxSlots - 1;
        if (m_selectedIndex < 0)         m_selectedIndex = 0;
    }

    int Inventory::GetHotbarLevel() const { return mHotbarLevel; }

    int Inventory::GetHotbarSlotCount() const
    {
        int slots = kHotbarSlots + (mHotbarLevel * 2);
        if (slots < kHotbarSlots)    slots = kHotbarSlots;
        if (slots > MaxHotbarSlots)  slots = MaxHotbarSlots;
        return slots;
    }

    // =========================================================
    // BACKPACK
    // =========================================================

    void Inventory::SetBackpackLevel(int level)
    {
        if (level < 1) level = 1;
        if (level > 3) level = 3;
        mBackpackLevel = level;
    }

    int Inventory::GetBackpackLevel() const { return mBackpackLevel; }
    int Inventory::GetBackpackRowCount() const { return mBackpackLevel; }
    int Inventory::GetBackpackSlotCount() const { return GetBackpackRowCount() * kBackpackCols; }

    bool Inventory::IsBackpackOpen() const { return mBackpackOpen; }

    void Inventory::ToggleBackpack()
    {
        mBackpackOpen = !mBackpackOpen;
        if (!mBackpackOpen) CancelDrag();
    }

    void Inventory::SetBackpackOpen(bool open)
    {
        mBackpackOpen = open;
        if (!mBackpackOpen) CancelDrag();
    }

    const InvSlot& Inventory::GetBackpack(int idx) const
    {
        static InvSlot empty{ -1, 0, 0 };
        if (idx < 0 || idx >= kBackpackMaxSlots) return empty;
        return mBackpack[idx];
    }

    InvSlot& Inventory::GetBackpack(int idx) { return mBackpack[idx]; }

    // =========================================================
    // SLOT ACTIVE CHECK
    // =========================================================

    bool Inventory::IsSlotActive(SlotRef r) const
    {
        if (!r.IsValid()) return false;
        if (r.kind == SlotKind::Hotbar)
            return r.idx >= 0 && r.idx < GetHotbarSlotCount();
        if (r.kind == SlotKind::Backpack)
            return r.idx >= 0 && r.idx < GetBackpackSlotCount();

        if (r.kind == SlotKind::Trash)
            return true;

        return false;
    }

    // =========================================================
    // PICKUP: BACKPACK FIRST
    // =========================================================

    bool Inventory::TryAddItem_BackpackFirst(int itemId, GLuint iconTex, int count)
    {
        int hotSlots = GetHotbarSlotCount();
        for (int i = 0; i < hotSlots; ++i)
        {
            if (m_hotbar[i].itemId == -1)
            {
                m_hotbar[i] = { itemId, iconTex, count };
                return true;
            }
        }
        int backSlots = GetBackpackSlotCount();
        for (int i = 0; i < backSlots; ++i)
        {
            if (mBackpack[i].itemId == -1)
            {
                mBackpack[i] = { itemId, iconTex, count };
                return true;
            }
        }
        return false;
    }

    bool Inventory::IsInventoryFull() const
    {
        int backSlots = GetBackpackSlotCount();
        bool backFull = true;
        for (int i = 0; i < backSlots; ++i)
            if (mBackpack[i].itemId == -1) { backFull = false; break; }

        int hotSlots = GetHotbarSlotCount();
        bool hotFull = true;
        for (int i = 0; i < hotSlots; ++i)
            if (m_hotbar[i].itemId == -1) { hotFull = false; break; }

        return backFull && hotFull;
    }

    // =========================================================
    // DRAG & DROP
    // =========================================================

    bool    Inventory::IsDragging() const { return mDragging; }
    SlotRef Inventory::GetDragFrom() const { return mDragFrom; }

    InvSlot Inventory::GetDraggedItemCopy() const
    {
        if (!mDragging || !mDragFrom.IsValid()) return InvSlot{ -1, 0, 0 };
        if (mDragFrom.kind == SlotKind::Hotbar)   return Get(mDragFrom.idx);
        if (mDragFrom.kind == SlotKind::Backpack) return GetBackpack(mDragFrom.idx);
        return InvSlot{ -1, 0, 0 };
    }

    void Inventory::BeginDrag(SlotRef from)
    {
        if (!IsSlotActive(from)) return;
        const InvSlot* s = nullptr;
        if (from.kind == SlotKind::Hotbar)   s = &Get(from.idx);
        if (from.kind == SlotKind::Backpack) s = &GetBackpack(from.idx);
        if (!s || s->itemId < 0) return;
        mDragging = true;
        mDragFrom = from;
    }

    void Inventory::CancelDrag()
    {
        mDragging = false;
        mDragFrom = {};
    }

    void Inventory::DropOn(SlotRef to)
    {
        if (!mDragging || !mDragFrom.IsValid())
            return;

        if (!IsSlotActive(to))
        {
            CancelDrag();
            return;
        }

        // Dropping onto the same source slot = do nothing
        if (to.kind == mDragFrom.kind && to.idx == mDragFrom.idx)
        {
            CancelDrag();
            return;
        }

        auto getSlotPtr = [&](SlotRef r) -> InvSlot*
            {
                if (r.kind == SlotKind::Hotbar)   return &Get(r.idx);
                if (r.kind == SlotKind::Backpack) return &GetBackpack(r.idx);
                return nullptr;
            };

        InvSlot* src = getSlotPtr(mDragFrom);
        if (!src)
        {
            CancelDrag();
            return;
        }

        // -------------------------
        // TRASH: delete dragged item
        // -------------------------
        if (to.kind == SlotKind::Trash)
        {
            *src = InvSlot{ -1, 0, 0 };
            CancelDrag();
            return;
        }

        InvSlot* dst = getSlotPtr(to);
        if (!dst)
        {
            CancelDrag();
            return;
        }

        if (dst->itemId < 0) { *dst = *src; *src = InvSlot{ -1, 0, 0 }; }
        else { std::swap(*src, *dst); }

        CancelDrag();
    }

    // =========================================================
    // EXPAND RECIPE TO BASE INGREDIENTS
    // Non-static free function - exposed in header so Lua
    // registration code in ScriptingSystem can call it directly.
    //
    // Recursively walks the recipe tree to find the raw base
    // ingredients (items with no recipe) needed to craft itemId.
    // =========================================================
    RecipeRequirements ExpandRecipeToBase(
        int itemId,
        int quantity,
        std::unordered_set<int> visited)
    {
        RecipeRequirements result;

        // Circular dependency guard
        if (visited.count(itemId)) {
            std::cerr << "[Recipe] Circular dependency detected for item " << itemId << "\n";
            result.valid = false;
            return result;
        }
        visited.insert(itemId);

        // Look for a recipe that produces this item
        const PotionRecipe* recipe = GetRecipeForResult(itemId);

        // No recipe - it IS a base ingredient
        if (!recipe) {
            result.baseIngredients[itemId] += quantity;
            return result;
        }

        // Recursively expand each ingredient in the recipe
        for (int ingredientId : recipe->ingredientIDs) {
            auto subReq = ExpandRecipeToBase(ingredientId, quantity, visited);

            if (!subReq.valid) {
                result.valid = false;
                return result;
            }

            for (const auto& [baseId, count] : subReq.baseIngredients) {
                result.baseIngredients[baseId] += count;
            }
        }

        return result;
    }

    // =========================================================
    // CAN CRAFT RECIPE
    // Checks full inventory (hotbar + backpack) against the
    // expanded base ingredient requirements.
    // =========================================================
    bool CanCraftRecipe(const PotionRecipe& recipe)
    {
        auto& inv = Inventory::Instance();

        // Build available map from hotbar + backpack
        std::unordered_map<int, int> available;
        for (int i = 0; i < inv.GetHotbarSlotCount(); ++i) {
            const auto& slot = inv.Get(i);
            if (slot.itemId >= 0)
                available[slot.itemId] += (slot.count > 0 ? slot.count : 1);
        }
        for (int i = 0; i < inv.GetBackpackSlotCount(); ++i) {
            const auto& slot = inv.GetBackpack(i);
            if (slot.itemId >= 0)
                available[slot.itemId] += (slot.count > 0 ? slot.count : 1);
        }

        // Check direct recipe inputs first (e.g. iron_bar + copper_bar)
        bool directSatisfied = true;
        for (int ingredientId : recipe.ingredientIDs) {
            if (available[ingredientId] < 1) { directSatisfied = false; break; }
        }
        if (directSatisfied) return true;

        // Fall back to expanded base ingredients (e.g. iron_ore + copper_ore)
        RecipeRequirements req = ExpandRecipeToBase(recipe.resultItemID);
        if (!req.valid) return false;
        for (const auto& [itemId, needed] : req.baseIngredients) {
            if (available[itemId] < needed) return false;
        }
        return true;
    }

    // =========================================================
    // GET CRAFTABLE POTION NAMES
    // Returns names of all potions the player can craft right now.
    // =========================================================
    std::vector<std::string> GetCraftablePotionNames()
    {
        const auto& recipes = GetGlobalRecipes();
        std::vector<std::string> craftable;
        std::unordered_set<std::string> added;

        // Potions craftable from ingredients
        for (const auto& recipe : recipes) 
        {
            if (CanCraftRecipe(recipe) &&
                GetItemType(recipe.resultItemID) == ItemType::POTION ||
                GetItemType(recipe.resultItemID) == ItemType::POWDER) 
            {
                std::string name = kItemNames[recipe.resultItemID];
                if (added.insert(name).second)
                    craftable.push_back(name);
            }
        }

        // Finished potions already in inventory
        auto& inv = Inventory::Instance();
        auto checkSlots = [&](auto getSlot, int count) {
            for (int i = 0; i < count; ++i) {
                const auto& slot = getSlot(i);
                if (slot.itemId >= 0 &&
                    GetItemType(slot.itemId) == ItemType::POTION) {
                    std::string name = kItemNames[slot.itemId];
                    if (added.insert(name).second)
                        craftable.push_back(name);
                }
            }
            };
        checkSlots([&](int i) { return inv.Get(i); }, inv.GetHotbarSlotCount());
        checkSlots([&](int i) { return inv.GetBackpack(i); }, inv.GetBackpackSlotCount());

        return craftable;
    }

    // =========================================================
    // CLEAR ALL
    // =========================================================

    void Inventory::ClearAll()
    {
        for (auto& slot : m_hotbar)  slot = { -1, 0, 0 };
        for (auto& slot : mBackpack) slot = { -1, 0, 0 };
        m_selectedIndex = 0;
        mBackpackOpen = false;
        CancelDrag();
        std::cout << "[Inventory] ClearAll: items cleared, levels preserved.\n";
    }
} // namespace Framework
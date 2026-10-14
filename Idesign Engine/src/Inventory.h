#pragma once
#include "pch.h"
#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <glm.hpp>
#include <FontSystem.h>
#include "ItemComponent.h"
#include "AudioSystem.h"

namespace Framework {

    // for backpack size
    constexpr int kBackpackCols = 6;
    constexpr int kBackpackRowsMax = 10;
    constexpr int kBackpackMaxSlots = kBackpackCols * kBackpackRowsMax;

    enum class SlotKind : uint8_t { Hotbar, Backpack, Trash, None };

    struct SlotRef
    {
        SlotKind kind = SlotKind::None;
        int idx = -1;
        bool IsValid() const { return kind != SlotKind::None && idx >= 0; }
    };

    // Keep hotbar size in one place
    constexpr int MaxHotbarSlots = 10;
    constexpr int kHotbarSlots = 5;

    // One slot = a single non-stackable item
    struct InvSlot {
        int    itemId = -1;
        GLuint iconTex = 0;
        int    count = 0;
    };

    // =========================================================
    // RECIPE REQUIREMENTS
    // Holds the expanded base ingredient map for a recipe.
    // Exposed here so Lua registration code can use it directly.
    // =========================================================
    struct RecipeRequirements {
        std::unordered_map<int, int> baseIngredients; // itemId -> count needed
        bool valid = true; // false if circular dependency detected
    };

    // =========================================================
    // ExpandRecipeToBase
    // Recursively expands a recipe to find the raw base
    // ingredients needed to craft the given itemId.
    // Exposed (non-static) so Lua registration can call it.
    // =========================================================
    RecipeRequirements ExpandRecipeToBase(
        int itemId,
        int quantity = 1,
        std::unordered_set<int> visited = {}
    );

    // =========================================================
    // CanCraftRecipe
    // Checks if the player's current inventory can satisfy
    // all base ingredient requirements for the given recipe.
    // =========================================================
    bool CanCraftRecipe(const PotionRecipe& recipe);

    // =========================================================
    // GetCraftablePotionNames
    // Returns names of all potions the player can currently craft.
    // =========================================================
    std::vector<std::string> GetCraftablePotionNames();

    // =========================================================
    // INVENTORY CLASS
    // =========================================================
    class Inventory {
    public:
        static Inventory& Instance();

        const InvSlot& Get(int idx) const {
            static InvSlot empty{ -1, 0, 0 };
            if (idx < 0 || idx >= MaxHotbarSlots) return empty;
            return m_hotbar[idx];
        }
        InvSlot& Get(int idx) { return m_hotbar[idx]; }

        // Core API
        int  AddFirstEmpty(int itemId, GLuint iconTex = 0, int count = 1);
        void SetHotbarSlot(int idx, int itemId, GLuint iconTex = 0, int count = 1);
        void ClearHotbarSlot(int idx);

        // Selection
        int  GetSelectedIndex() const { return m_selectedIndex; }
        void SetSelectedIndex(int idx);
        void ChangeSelected(int delta);
        int  GetSelectedItemId() const;

        // Hotbar upgrade
        void SetHotbarLevel(int level);
        int  GetHotbarLevel() const;
        int  GetHotbarSlotCount() const;

        // Backpack
        void SetBackpackLevel(int level);
        int  GetBackpackLevel() const;
        int  GetBackpackSlotCount() const;
        int  GetBackpackRowCount() const;

        bool IsBackpackOpen() const;
        void ToggleBackpack();
        void SetBackpackOpen(bool open);

        const InvSlot& GetBackpack(int idx) const;
        InvSlot& GetBackpack(int idx);

        // Drag & drop
        bool    IsDragging() const;
        SlotRef GetDragFrom() const;
        InvSlot GetDraggedItemCopy() const;

        void BeginDrag(SlotRef from);
        void CancelDrag();
        void DropOn(SlotRef to);

        bool IsSlotActive(SlotRef r) const;

        bool TryAddItem_BackpackFirst(int itemId, GLuint iconTex, int count);
        bool IsInventoryFull() const;

        // Resets all items and upgrade levels to defaults
        void ClearAll();  

        void ResetBackpackLevel() { mBackpackLevel = 0; }

    private:
        Inventory() = default;

        std::array<InvSlot, MaxHotbarSlots>    m_hotbar{};
        std::array<InvSlot, kBackpackMaxSlots> mBackpack{};

        int  m_selectedIndex = 0;
        int  mHotbarLevel = 1;
        int  mBackpackLevel = 0;
        bool mBackpackOpen = false;

        bool    mDragging = false;
        SlotRef mDragFrom{};
    };

} // namespace Framework
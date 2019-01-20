#include "menu.hpp"

extern "C"
{
#include "style.h"
}
#include "config.hpp"
#include "pkgi.hpp"

static int menu_search_clear;

static Config menu_config;
static uint32_t menu_selected;
static int menu_allow_refresh;

static MenuResult menu_result;

static int32_t menu_width;
static int32_t menu_delta;

typedef enum
{
    MenuSearch,
    MenuSearchClear,
    MenuText,
    MenuSort,
    MenuFilter,
    MenuRefresh,
    MenuShow,
    MenuButton,
} MenuType;

typedef struct
{
    MenuType type;
    const char* text;
    uint32_t value;
} MenuEntry;

static const MenuEntry menu_entries[] = {
        {MenuSearch, "搜索...", 0},
        {MenuSearchClear, PKGI_UTF8_CLEAR " 取消搜索", 0},

        {MenuText, "排序順序:", 0},
        {MenuSort, "游戲編號", SortByTitle},
        {MenuSort, "發行區域", SortByRegion},
        {MenuSort, "游戲名稱", SortByName},
        {MenuSort, "游戲容量", SortBySize},
        {MenuSort, "更新時間", SortByDate},

        {MenuText, "篩選項:", 0},
        {MenuFilter, "亞洲", DbFilterRegionASA},
        {MenuFilter, "歐洲", DbFilterRegionEUR},
        {MenuFilter, "日本", DbFilterRegionJPN},
        {MenuFilter, "美國", DbFilterRegionUSA},
        {MenuFilter, "已安裝的游戲", DbFilterInstalled},

        {MenuRefresh, "刷新", 0},

        {MenuShow, "顯示PSV游戲", 1},
        {MenuShow, "顯示PSV追加下載内容", 2},
        {MenuShow, "顯示PSX游戲", 4},
        {MenuShow, "顯示PSP游戲", 8},
        {MenuShow, "顯示PSM游戲", 16},

        {MenuButton, "关于", 0},
};

int pkgi_menu_is_open(void)
{
    return menu_width != 0;
}

MenuResult pkgi_menu_result()
{
    return menu_result;
}

void pkgi_menu_get(Config* config)
{
    *config = menu_config;
}

void pkgi_menu_start(int search_clear, const Config* config, int allow_refresh)
{
    menu_search_clear = search_clear;
    menu_width = 1;
    menu_delta = 1;
    menu_config = *config;
    menu_allow_refresh = allow_refresh;
}

int pkgi_do_menu(pkgi_input* input)
{
    if (menu_delta != 0)
    {
        menu_width +=
                menu_delta *
                (int32_t)(input->delta * PKGI_ANIMATION_SPEED / 2 / 1000000);

        if (menu_delta < 0 && menu_width <= 0)
        {
            menu_width = 0;
            menu_delta = 0;
            return 0;
        }
        else if (menu_delta > 0 && menu_width >= PKGI_MENU_WIDTH)
        {
            menu_width = PKGI_MENU_WIDTH;
            menu_delta = 0;
        }
    }

    if (menu_width != 0)
    {
        pkgi_draw_rect(
                VITA_WIDTH - menu_width,
                0,
                menu_width,
                VITA_HEIGHT,
                PKGI_COLOR_MENU_BACKGROUND);
    }

    if (input->active & PKGI_BUTTON_UP)
    {
        do
        {
            if (menu_selected == 0)
            {
                menu_selected = PKGI_COUNTOF(menu_entries) - 1;
            }
            else
            {
                menu_selected--;
            }
        } while (menu_entries[menu_selected].type == MenuText ||
                 (menu_entries[menu_selected].type == MenuSearchClear &&
                  !menu_search_clear) ||
                 (menu_entries[menu_selected].type == MenuShow &&
                  !(menu_entries[menu_selected].value & menu_allow_refresh)));
    }

    if (input->active & PKGI_BUTTON_DOWN)
    {
        do
        {
            if (menu_selected == PKGI_COUNTOF(menu_entries) - 1)
            {
                menu_selected = 0;
            }
            else
            {
                menu_selected++;
            }
        } while (menu_entries[menu_selected].type == MenuText ||
                 (menu_entries[menu_selected].type == MenuSearchClear && !menu_search_clear) ||
                 (menu_entries[menu_selected].type == MenuShow && !(menu_entries[menu_selected].value & menu_allow_refresh)));
    }

    if (input->pressed & pkgi_cancel_button())
    {
        menu_result = MenuResultCancel;
        menu_delta = -1;
        return 1;
    }
    else if (input->pressed & PKGI_BUTTON_T)
    {
        menu_result = MenuResultAccept;
        menu_delta = -1;
        return 1;
    }
    else if (input->pressed & pkgi_ok_button())
    {
        MenuType type = menu_entries[menu_selected].type;
        if (type == MenuSearch)
        {
            menu_result = MenuResultSearch;
            menu_delta = -1;
            return 1;
        }
        else if (type == MenuSearchClear)
        {
            menu_selected--;
            menu_result = MenuResultSearchClear;
            menu_delta = -1;
            return 1;
        }
        else if (type == MenuRefresh)
        {
            menu_result = MenuResultRefresh;
            menu_delta = -1;
            return 1;
        }
        else if (type == MenuButton)
        {
            menu_result = MenuAbout;
            menu_delta = -1;
            return 1;
        }
        else if (type == MenuShow)
        {
            switch (menu_entries[menu_selected].value)
            {
            case 1:
                menu_result = MenuResultShowGames;
                break;
            case 2:
                menu_result = MenuResultShowDlcs;
                break;
            case 4:
                menu_result = MenuResultShowPsxGames;
                break;
            case 8:
                menu_result = MenuResultShowPspGames;
                break;
            case 16:
                menu_result = MenuResultShowPsmGames;
                break;
            }

            menu_delta = -1;
            return 1;
        }
        else if (type == MenuSort)
        {
            DbSort value = (DbSort)menu_entries[menu_selected].value;
            if (menu_config.sort == value)
            {
                menu_config.order = menu_config.order == SortAscending
                                            ? SortDescending
                                            : SortAscending;
            }
            else
            {
                menu_config.sort = value;
            }
        }
        else if (type == MenuFilter)
        {
            menu_config.filter ^= menu_entries[menu_selected].value;
        }
    }

    if (menu_width != PKGI_MENU_WIDTH)
    {
        return 1;
    }

    int font_height = pkgi_text_height("M");

    int y = PKGI_MENU_TOP_PADDING;
    for (uint32_t i = 0; i < PKGI_COUNTOF(menu_entries); i++)
    {
        const MenuEntry* entry = menu_entries + i;

        MenuType type = entry->type;
        if (type == MenuText)
        {
            y += font_height / 2;
        }
        else if (type == MenuSearchClear && !menu_search_clear)
        {
            continue;
        }
        else if (type == MenuRefresh)
            y += font_height / 3;
        else if (type == MenuShow)
        {
            if (entry[-1].type != MenuShow)
                y += font_height / 3;
            if (!(entry->value & menu_allow_refresh))
            {
                continue;
            }
        }

        uint32_t color = menu_selected == i ? PKGI_COLOR_TEXT_MENU_SELECTED
                                            : PKGI_COLOR_TEXT_MENU;

        int x = VITA_WIDTH - PKGI_MENU_WIDTH + PKGI_MENU_LEFT_PADDING;

        char text[64];
        if (type == MenuSearch || type == MenuSearchClear || type == MenuText ||
            type == MenuRefresh || type == MenuShow || type == MenuButton)
        {
            pkgi_strncpy(text, sizeof(text), entry->text);
        }
        else if (type == MenuSort)
        {
            if (menu_config.sort == (DbSort)entry->value)
            {
                pkgi_snprintf(
                        text,
                        sizeof(text),
                        "%s %s",
                        menu_config.order == SortAscending
                                ? PKGI_UTF8_SORT_ASC
                                : PKGI_UTF8_SORT_DESC,
                        entry->text);
            }
            else
            {
                x += pkgi_text_width(PKGI_UTF8_SORT_ASC " ");
                pkgi_strncpy(text, sizeof(text), entry->text);
            }
        }
        else if (type == MenuFilter)
        {
            pkgi_snprintf(
                    text,
                    sizeof(text),
                    "%s %s",
                    menu_config.filter & entry->value ? PKGI_UTF8_CHECK_ON
                                                      : PKGI_UTF8_CHECK_OFF,
                    entry->text);
        }
        pkgi_draw_text(x, y, color, text);

        y += font_height;
    }

    return 1;
}

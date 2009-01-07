/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *			 Shop and repair shop module			    *
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"



/*
 * Local functions
 */

#define	CD	CHAR_DATA
CD *	find_keeper	args( ( CHAR_DATA *ch ) );
CD *	find_fixer	args( ( CHAR_DATA *ch ) );
int	get_cost	args( ( CHAR_DATA *ch, CHAR_DATA *keeper,
            OBJ_DATA *obj, bool fBuy ) );
int 	get_repaircost  args( ( CHAR_DATA *keeper, OBJ_DATA *obj ) );
#undef CD

/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *keeper, *whof;
    SHOP_DATA *pShop;
    int speakswell;

    pShop = NULL;
    for ( keeper = ch->in_room->first_person;
            keeper;
            keeper = keeper->next_in_room )
        if ( IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL )
            break;

    if ( !pShop )
    {
        send_to_char( "Nie mozesz tu tego zrobic.\n\r", ch );
        return NULL;
    }

    /*
     * Undesirables.
     */
    if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_KILLER) )
    {
        do_say( keeper, "Zabojcy nie sa mile widziani!" );
        if (ch->sex ==2 )
            sprintf( buf, "%s ZABOJCZYNI jest tutaj!\n\r", ch->name );
        else
            sprintf( buf, "%s ZABOJCA jest tutaj!\n\r", ch->name );
        do_shout( keeper, buf );
        return NULL;
    }

    if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_THIEF) )
    {
        do_say( keeper, "Zlodzieje nie sa mile widziani!" );
        if (ch->sex == 2)
            sprintf( buf, "%s ZLODZIEJKA jest tutaj!\n\r", ch->name );
        else
            sprintf( buf, "%s ZLODZIEJ jest tutaj!\n\r", ch->name );
        do_shout( keeper, buf );
        return NULL;
    }

    /*
     * Disallow sales during battle
     */
    if ( (whof=who_fighting(keeper)) != NULL )
    {
        if ( whof == ch )
            send_to_char( "To nie jest dobry pomysl...\n\r", ch );
        else
            do_say( keeper, "Przepraszam, ale jestem zajety!" );
        return NULL;
    }

    if ( who_fighting(ch) )
    {
        ch_printf( ch, "%s ma teraz inne zajecie.\n\r", PERS(keeper, ch) );
        return NULL;
    }

    /*
     * Check to see if show is open.
     * Supports closing times after midnight
     */
    if ( pShop->open_hour > pShop->close_hour )
    {
        if ( time_info.hour < pShop->open_hour
                &&   time_info.hour > pShop->close_hour )
        {
            do_say( keeper, "Sorry, przyjdz pozniej." );
            return NULL;
        }
    }
    else
    {
        if ( time_info.hour < pShop->open_hour )
        {
            do_say( keeper, "Sorry, przyjdz pozniej." );
            return NULL;
        }
        if ( time_info.hour > pShop->close_hour )
        {
            do_say( keeper, "Sorry, przyjdz jutro." );
            return NULL;
        }
    }

    if ( keeper->position == POS_SLEEPING )
    {
        send_to_char( "Przeciez spi!\n\r", ch );
        return NULL;
    }

    if ( keeper->position < POS_SLEEPING )
    {
        if (keeper->sex == 2)
            send_to_char( "Nie sadze by Cie uslyszala...\n\r", ch );
        else
            send_to_char( "Nie sadze by Cie uslyszal...\n\r", ch );
        return NULL;
    }

    /*
     * Invisible or hidden people.
     */
    if ( !can_see( keeper, ch ) && !IS_IMMORTAL( ch ) )
    {
        do_say( keeper, "Nie handluje z kims kogo nie widze." );
        return NULL;
    }

    speakswell = UMIN(knows_language(keeper, ch->speaking, ch),
            knows_language(ch, ch->speaking, keeper));

    if ( (number_percent() % 65) > speakswell )
    {
        if ( speakswell > 60 )
            sprintf( buf, "%s Mozna jeszcze raz?  Nie do konca lapie watek.", ch->name );
        else
            if ( speakswell > 50 )
                sprintf( buf, "%s Czy mozesz mowic bardziej wyraznie?", ch->name );
            else
                if ( speakswell > 40 )
                    sprintf( buf, "%s Sorry... ale czego Ty wlasciwie chcesz?", ch->name );
                else
                    sprintf( buf, "%s Nie moge Cie zrozumiec.", ch->name );
        do_tell( keeper, buf );
        return NULL;
    }

    return keeper;
}

/*
 * repair commands.
 */
CHAR_DATA *find_fixer( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *keeper, *whof;
    REPAIR_DATA *rShop;
    int speakswell;

    rShop = NULL;
    for ( keeper = ch->in_room->first_person;
            keeper;
            keeper = keeper->next_in_room )
        if ( IS_NPC(keeper) && (rShop = keeper->pIndexData->rShop) != NULL )
            break;

    if ( !rShop )
    {
        send_to_char( "Nie mozesz tu tego uczynic.\n\r", ch );
        return NULL;
    }

    /*
     * Undesirables.
     */
    if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_KILLER) )
    {
        do_say( keeper, "Mordercy nie sa tu mile widziani!" );
        if (ch->sex == 2)
            sprintf( buf, "%s MORDERCZYNI jest tutaj!\n\r", ch->name );
        else
            sprintf( buf, "%s MORDERCA jest tutaj!\n\r", ch->name );
        do_shout( keeper, buf );
        return NULL;
    }

    if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_THIEF) )
    {
        do_say( keeper, "Zlodzieje nie sa tu mile widziani!" );
        if (ch->sex == 2)
            sprintf( buf, "%s ZLODZIEJKA jest tutaj!\n\r", ch->name );
        else
            sprintf( buf, "%s ZLODZIEJ jest tutaj!\n\r", ch->name );
        do_shout( keeper, buf );
        return NULL;
    }

    /*
     * Disallow sales during battle
     */
    if ( (whof=who_fighting(keeper)) != NULL )
    {
        if ( whof == ch )
            send_to_char( "To nie jest dobry pomysl...\n\r", ch );
        else
            do_say( keeper, "Mam teraz inne zajecie!" );
        return NULL;
    }

    /* According to rlog, this is the second time I've done this
     * so mobiles can repair in combat.  -- Blod, 1/98
     */
    if ( !IS_NPC( ch ) && who_fighting( ch ) )
    {
        ch_printf( ch, "%s ma teraz inne zajecie.\n\r", PERS(keeper, ch) );
        return NULL;
    }

    /*
     * Check to see if show is open.
     * Supports closing times after midnight
     */
    if ( rShop->open_hour > rShop->close_hour )
    {
        if ( time_info.hour < rShop->open_hour
                &&   time_info.hour > rShop->close_hour )
        {
            do_say( keeper, "Sorry, przyjdz pozniej." );
            return NULL;
        }
    }
    else
    {
        if ( time_info.hour < rShop->open_hour )
        {
            do_say( keeper, "Sorry, przyjdz pozniej." );
            return NULL;
        }
        if ( time_info.hour > rShop->close_hour )
        {
            do_say( keeper, "Sorry, przyjdz moze jutro." );
            return NULL;
        }
    }

    if ( keeper->position == POS_SLEEPING )
    {
        send_to_char( "Przeciez spi!\n\r", ch );
        return NULL;
    }

    if ( keeper->position < POS_SLEEPING )
    {
        send_to_char( "Nie moze Cie uslyszec...\n\r", ch );
        return NULL;
    }

    /*
     * Invisible or hidden people.
     */
    if ( !can_see( keeper, ch ) && !IS_IMMORTAL( ch ))
    {
        do_say( keeper, "Nie handluje z kims kogo nie widze." );
        return NULL;
    }

    speakswell = UMIN(knows_language(keeper, ch->speaking, ch),
            knows_language(ch, ch->speaking, keeper));

    if ( (number_percent() % 65) > speakswell )
    {
        if ( speakswell > 60 )
            sprintf( buf, "%s Mozesz powtorzyc?  Nie do konca lapie watek.", ch->name );
        else
            if ( speakswell > 50 )
                sprintf( buf, "%s Mozesz mowic troche wyrazniej?", ch->name );
            else
                if ( speakswell > 40 )
                    sprintf( buf, "%s Sorry... ale czego Ty wlasciwie chcesz?", ch->name );
                else
                    sprintf( buf, "%s Nie moge Cie zrozumiec.", ch->name );
        do_tell( keeper, buf );
        return NULL;
    }

    return keeper;
}



int get_cost( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy )
{
    SHOP_DATA *pShop;
    int cost;
    bool richcustomer;
    int profitmod;

    if ( !obj || ( pShop = keeper->pIndexData->pShop ) == NULL )
        return 0;

    if ( ch->gold > (ch->level * ch->level * 100000) )
        richcustomer = TRUE;
    else
        richcustomer = FALSE;

    if ( fBuy )
    {
        profitmod = 13 - get_curr_cha(ch) + (richcustomer ? 15 : 0)
            + ((URANGE(5,ch->level,LEVEL_AVATAR)-20)/2);
        cost = (int) (obj->cost
                * UMAX( (pShop->profit_sell+1), pShop->profit_buy+profitmod ) )
            / 100;

        /* Thanks to Nick Gammon for pointing out this line
           (it was the first line in this block, making it useless) */

        cost = (int) (cost * (80 + UMIN(ch->level, LEVEL_AVATAR))) / 100;

        switch(ch->race) /* racism... should compare against shopkeeper's race */
        {
            case(1): cost /=1.1; break;/* elf */
            case(2): cost /=0.97; break; /* dwarf */
            case(3):  cost /=1.02; break;/* halfling */
            case(4):  cost /=1.08; break;/* pixie */
            case(6):  cost /=0.92; break;/* half-ogre */
            case(7):  cost /=0.94; break;/* half-orc */
            case(8):  cost /=0.90; break;/* half-troll */
            case(9):  cost /=1.04; break;/* half-elf */
            case(10):  cost /=1.06; break;/* gith */
        }

    }
    else
    {
        OBJ_DATA *obj2;
        int itype;

        profitmod = get_curr_cha(ch) - 13 - (richcustomer ? 15 : 0);
        cost = 0;
        for ( itype = 0; itype < MAX_TRADE; itype++ )
        {
            if ( obj->item_type == pShop->buy_type[itype] )
            {
                cost = (int) (obj->cost
                        * UMIN( (pShop->profit_buy-1),
                            pShop->profit_sell+profitmod) ) / 100;
                break;
            }
        }

        for ( obj2 = keeper->first_carrying; obj2; obj2 = obj2->next_content )
        {
            if ( obj->pIndexData == obj2->pIndexData )
            {
                cost = 0;
                break;
            }
        }
    }

    if ( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
        cost = (int) (cost * obj->value[2] / obj->value[1]);

    return cost;
}

int get_repaircost( CHAR_DATA *keeper, OBJ_DATA *obj )
{
    REPAIR_DATA *rShop;
    int cost;
    int itype;
    bool found;

    if ( !obj || ( rShop = keeper->pIndexData->rShop ) == NULL )
        return 0;

    cost = 0;
    found = FALSE;
    for ( itype = 0; itype < MAX_FIX; itype++ )
    {
        if ( obj->item_type == rShop->fix_type[itype] )
        {
            cost = (int) (obj->cost * rShop->profit_fix / 1000);
            found = TRUE;
            break;
        }
    }

    if ( !found )
        cost = -1;

    if ( cost == 0 )
        cost = 1;

    if ( found && cost > 0 )
    {
        switch (obj->item_type)
        {
            case ITEM_ARMOR:
                if (obj->value[0] >= obj->value[1])
                    cost = -2;
                else
                    cost *= (obj->value[1] - obj->value[0]);
                break;
            case ITEM_WEAPON:
                if (INIT_WEAPON_CONDITION == obj->value[0])
                    cost = -2;
                else
                    cost *= (INIT_WEAPON_CONDITION - obj->value[0]);
                break;
            case ITEM_WAND:
            case ITEM_STAFF:
                if (obj->value[2] >= obj->value[1])
                    cost = -2;
                else
                    cost *= (obj->value[1] - obj->value[2]);
        }
    }

    return cost;
}



void do_buy( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int maxgold;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Co chcesz kupic?\n\r", ch );
        return;
    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
        char buf[MAX_STRING_LENGTH];
        CHAR_DATA *pet;
        ROOM_INDEX_DATA *pRoomIndexNext;
        ROOM_INDEX_DATA *in_room;

        if ( IS_NPC(ch) )
            return;

        pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
        if ( !pRoomIndexNext )
        {
            bug( "Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum );
            send_to_char( "Nie mozesz tu tego kupic.\n\r", ch );
            return;
        }

        in_room     = ch->in_room;
        ch->in_room = pRoomIndexNext;
        pet         = get_char_room( ch, arg );
        ch->in_room = in_room;

        if ( pet == NULL || !IS_NPC( pet ) || !xIS_SET(pet->act, ACT_PET) )
        {
            send_to_char( "Nie mozesz tu tego kupic.\n\r", ch );
            return;
        }

        if ( xIS_SET(ch->act, PLR_BOUGHT_PET) )
        {
            send_to_char( "Kupowales juz zwierzaka na tym poziomie.\n\r", ch );
            return;
        }

        if ( ch->gold < 10 * pet->level * pet->level )
        {
            send_to_char( "Nie stac Cie.\n\r", ch );
            return;
        }

        if ( ch->level < pet->level )
        {
            send_to_char( "Nie jestes gotow by miec tego zwierzaka.\n\r", ch );
            return;
        }

        maxgold = 10 * pet->level * pet->level;
        ch->gold	-= maxgold;
        boost_economy( ch->in_room->area, maxgold );
        pet		= create_mobile( pet->pIndexData );
        xSET_BIT(ch->act, PLR_BOUGHT_PET);
        xSET_BIT(pet->act, ACT_PET);
        xSET_BIT(pet->affected_by, AFF_CHARM);
        /*	This isn't needed anymore since you can order your pets --Shaddai
            xSET_BIT(pet->affected_by, AFF_CHARM);
            */

        argument = one_argument( argument, arg );
        if ( arg[0] != '\0' )
        {
            sprintf( buf, "%s %s", pet->name, arg );
            STRFREE( pet->name );
            pet->name = STRALLOC( buf );
        }

        sprintf( buf, "%s ma na obrozy napis 'Naleze do %s'.\n\r",
                pet->description, ch->ncel );
        STRFREE( pet->description );
        pet->description = STRALLOC( buf );

        char_to_room( pet, ch->in_room );
        add_follower( pet, ch );
        send_to_char( "Powodzenia w wychowku.\n\r", ch );
        act( AT_ACTION, "$n kupuje $B.", ch, NULL, pet, TO_ROOM );
        return;
    }
    else
    {
        CHAR_DATA *keeper;
        OBJ_DATA *obj;
        int cost;
        int noi = 1;		/* Number of items */
        sh_int mnoi = 20;	/* Max number of items to be bought at once */

        if ( ( keeper = find_keeper( ch ) ) == NULL )
            return;

        maxgold = keeper->level * keeper->level * 50000;

        if ( is_number( arg ) )
        {
            noi = atoi( arg );
            argument = one_argument( argument, arg );
            if ( noi > mnoi )
            {
                act( AT_TELL, "$n mowi Ci 'Nie sprzedaje tylu rzeczy na"
                        " raz.'", keeper, NULL, ch, TO_VICT );
                ch->reply = keeper;
                return;
            }
        }

        obj  = get_obj_carry( keeper, arg );
        cost = ( get_cost( ch, keeper, obj, TRUE ) * noi );
        if ( cost <= 0 || !can_see_obj( ch, obj ) )
        {
            act( AT_TELL, "$n mowi Ci 'Nie sprzedaje czegos takiego -- sprobuj wpisac 'list'.'",
                    keeper, NULL, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( !IS_OBJ_STAT( obj, ITEM_INVENTORY ) && ( noi > 1 ) )
        {
            interpret( keeper, "laugh" );
            act( AT_TELL, "$n mowi Ci 'Mam w magazynie tego tylko tyle"
                    " by sprzedac jedno na raz.'", keeper, NULL, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( ch->gold < cost )
        {
            act( AT_TELL, "$n mowi Ci 'Nie stac Cie by kupic $v.'",
                    keeper, obj, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( obj->level > ch->level )
        {
            act( AT_TELL, "$n mowi Ci '$p ma zbyt wysoki poziom jak dla Ciebie.'",
                    keeper, obj, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( IS_OBJ_STAT(obj, ITEM_PROTOTYPE) 
                && get_trust( ch ) < LEVEL_IMMORTAL )
        {
            act( AT_TELL, "$n mowi Ci 'To tylko prototyp! Nie moge ci tego sprzedac...'", 
                    keeper, NULL, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
        {
            send_to_char( "Nie mozesz uniesc tylu rzeczy.\n\r", ch );
            return;
        }

        if ( ch->carry_weight + ( get_obj_weight( obj ) * noi )
                + (noi > 1 ? 2 : 0) > can_carry_w( ch ) )
        {
            send_to_char( "Nie mozesz uniesc tak ciezkich rzeczy.\n\r", ch );
            return;
        }

        if ( noi == 1 )
        {
            act( AT_ACTION, "$n kupuje $v.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "Kupujesz $v.", ch, obj, NULL, TO_CHAR );
        }
        else
        {
            sprintf( arg, "$n kupuje %d sztuk $f.", noi);
            act( AT_ACTION, arg, ch, obj, NULL, TO_ROOM );
            sprintf( arg, "Kupujesz %d sztuk $f.", noi);
            act( AT_ACTION, arg, ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$N wklada kupione przedmioty do sakiewki i wrecza Ci je.",
                    ch, NULL, keeper, TO_CHAR );
        }

        ch->gold     -= cost;
        keeper->gold += cost;

        if ( keeper->gold > maxgold )
        {
            boost_economy( keeper->in_room->area, keeper->gold - maxgold/2 );
            keeper->gold = maxgold/2;
            act( AT_ACTION, "$n chowa czesc zlota do wielkiej kasy.", keeper, NULL, NULL, TO_ROOM );
        }

        if ( IS_OBJ_STAT( obj, ITEM_INVENTORY ) )
        {
            OBJ_DATA *buy_obj, *bag;

            buy_obj = create_object( obj->pIndexData, obj->level );

            /*
             * Due to grouped objects and carry limitations in SMAUG
             * The shopkeeper gives you a bag with multiple-buy,
             * and also, only one object needs be created with a count
             * set to the number bought.		-Thoric
             */
            if ( noi > 1 )
            {
                bag = create_object( get_obj_index( OBJ_VNUM_SHOPPING_BAG ), 1 );
                xSET_BIT(bag->extra_flags, ITEM_GROUNDROT);
                bag->timer = 10; /* Blodkai, 4/97 */
                /* perfect size bag ;) */
                bag->value[0] = bag->weight + (buy_obj->weight * noi);
                buy_obj->count = noi;
                obj->pIndexData->count += (noi - 1);
                numobjsloaded += (noi - 1);
                obj_to_obj( buy_obj, bag );
                obj_to_char( bag, ch );
            }
            else
                obj_to_char( buy_obj, ch );
        }
        else
        {
            obj_from_char( obj );
            obj_to_char( obj, ch );
        }

        return;
    }
}

/*
 * This is a new list function which allows level limits to follow as
 * arguments.  This code relies heavily on the items held by the shopkeeper
 * being sorted in descending order by level.  obj_to_char in handler.c was
 * modified to achieve this.  Anyways, this list command will now throw flags
 * at the levels passed as arguments.  This helps pick out equipment which is
 * usable by the char, etc.  This was done rather than just producing a list
 * of equip at desired level because there would be an inconsistency between
 * #.item on one list and #.item on the other.
 * Syntax:
 *      list            -       list the items for sale, should be sorted by
 *                              level
 *      list #          -       list items and throw a flag at #
 *      list #1 #2      -       list items and throw flags at #1 and #2
 * Note that this won't work in pets stores. Since you can't control
 * the order in which the pets repop you can't guarantee a sorted list.
 * Last Modified: May 25, 1997 -- Fireblade
 */
void do_list( CHAR_DATA *ch, char *argument )
{
    /* Constants for producing the flags */
    char *divleft = "-----------------------------------[ ";
    char *divright = " ]-----------------------------------";


    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
        ROOM_INDEX_DATA *pRoomIndexNext;
        CHAR_DATA *pet;
        bool found;

        pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
        if ( !pRoomIndexNext )
        {
            bug( "Do_list: bad pet shop at vnum %d.", ch->in_room->vnum );
            send_to_char( "Nie mozesz tu tego uczynic.\n\r", ch );
            return;
        }

        found = FALSE;

        for ( pet = pRoomIndexNext->first_person; pet;
                pet = pet->next_in_room )
        {
            if ( xIS_SET(pet->act, ACT_PET) && IS_NPC(pet) )
            {
                if ( !found )
                {
                    found = TRUE;
                    send_to_pager( "Zwierzeta na sprzedaz:\n\r", ch );
                }
                pager_printf( ch, "[%2d] %8d - %s\n\r",
                        pet->level,
                        10 * pet->level * pet->level,
                        pet->short_descr );
            }
        }
        if ( !found )
            send_to_char( "Niestety, nie mamy teraz zadnych zwierzat na sprzedaz.\n\r", ch );
        return;
    }
    else
    {
        char arg[MAX_INPUT_LENGTH];
        char *rest;
        CHAR_DATA *keeper;
        OBJ_DATA *obj;
        int cost;
        bool found;
        /*      bool listall; */
        int lower, upper;

        rest = one_argument( argument, arg );

        if ( ( keeper = find_keeper( ch ) ) == NULL )
            return;

        found = FALSE;
        lower = -2;
        upper = -1;

        /* Get the level limits for the flags */
        if(is_number(arg))
        {
            lower = atoi(arg);
            rest = one_argument(rest, arg);

            if(is_number(arg))
            {
                upper = atoi(arg);
                rest = one_argument(rest,arg);
            }
        }

        /* Fix the limits if reversed */
        if(lower >= upper)
        {
            int temp;
            temp = lower;
            lower = upper;
            upper = temp;
        }

        /* Loop until you see an object higher level than char */
        /* Note that this depends on the keeper having a sorted list */
        for ( obj = keeper->first_carrying; obj;
                obj = obj->next_content )
        {
            if ( obj->wear_loc == WEAR_NONE
                    &&   can_see_obj( ch, obj )
                    && ( cost = get_cost( ch, keeper, obj, TRUE ) ) > 0
                    && ( arg[0] == '\0' || nifty_is_name( arg, obj->name ) ) )
            {
                if ( !found )
                {
                    found = TRUE;
                    send_to_pager( "[Lv  Cena] Przedmiot\n\r", ch );
                }

                if(obj->level <= upper)
                {
                    pager_printf(ch, "%s%2d%s\n\r", divleft, upper,
                            divright);
                    upper = -1;
                }

                if(obj->level < lower)
                {
                    pager_printf(ch, "%s%2d%s\n\r", divleft, lower,
                            divright);
                    lower = -1;
                }

                pager_printf( ch, "[%2d %5d] %s.\n\r",
                        obj->level, cost, capitalize( obj->short_descr ) );
            }
        }

        if(lower >= 0)
        {
            pager_printf(ch, "%s%2d%s\n\r", divleft, lower, divright);
        }

        if ( !found )
        {
            if ( arg[0] == '\0' )
                send_to_char( "Nie mozesz tu nic kupic.\n\r", ch );
            else
                send_to_char( "Nie mozesz tu tego kupic.\n\r", ch );
        }
        return;
    }
}

void do_sell( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Co chesz sprzedac?\n\r", ch );
        return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
        return;

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
        act( AT_TELL, "$n mowi Ci 'Nie masz czegos takiego.'",
                keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
        send_to_char( "Nie mozesz sie tego pozbyc!\n\r", ch );
        return;
    }

    if ( obj->timer > 0 )
    {
        act( AT_TELL, "$n mowi Ci '$p zbyt szybko traci na wartosci...'", keeper, obj, ch, TO_VICT );
        return;
    }

    if ( ( cost = get_cost( ch, keeper, obj, FALSE ) ) <= 0 )
    {
        act( AT_ACTION, "$n patrzy bez zainteresowania na $v.", keeper, obj, ch, TO_VICT );
        return;
    }

    if ( cost >= keeper->gold )
    {
        act( AT_TELL, "$n mowi Ci, '$p kosztuje wiecej niz moge zaplacic...'", keeper, obj, ch, TO_VICT );
        return;
    }

    separate_obj( obj );
    act( AT_ACTION, "$n sprzedaje $v.", ch, obj, NULL, TO_ROOM );
    sprintf( buf, "Sprzedajesz $v za %d szt. zlota.", cost );
    act( AT_ACTION, buf, ch, obj, NULL, TO_CHAR );
    ch->gold     += cost;
    keeper->gold -= cost;
    if ( keeper->gold < 0 )
        keeper->gold = 0;

    if ( obj->item_type == ITEM_TRASH )
        extract_obj( obj );
    else
    {
        obj_from_char( obj );
        obj_to_char( obj, keeper );
    }

    return;
}



void do_value( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Co chcesz wycenic?\n\r", ch );
        return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
        return;

    if ( ( obj = get_obj_carry( ch, argument ) ) == NULL )
    {
        act( AT_TELL, "$n mowi Ci 'Nie posiadasz czegos takiego.'",
                keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
        send_to_char( "Nie mozesz sie tego pozbyc!\n\r", ch );
        return;
    }

    if ( ( cost = get_cost( ch, keeper, obj, FALSE ) ) <= 0 )
    {
        act( AT_ACTION, "$n patrzy bez zainteresowania na $v.", keeper, obj, ch, TO_VICT );
        return;
    }

    sprintf( buf, "$n mowi Ci 'Moge Ci dac %d szt. zlota za $v.'", cost );
    act( AT_TELL, buf, keeper, obj, ch, TO_VICT );
    ch->reply = keeper;

    return;
}

/*
 * Repair a single object. Used when handling "repair all" - Gorog
 */
void repair_one_obj( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj,
        char *arg, int maxgold, char *fixstr, char*fixstr2 )
{
    char buf[MAX_STRING_LENGTH];
    int cost;

    if ( !can_drop_obj( ch, obj ) )
        ch_printf( ch, "Nie mozesz pozbyc sie %s.\n\r", obj->short_descr_dop );
    else if ( ( cost = get_repaircost( keeper, obj ) ) < 0 )
    {
        if (cost != -2)
            act( AT_TELL, "$n mowi Ci 'Niestety, nie moge nic uczynic z $y.'", 
                    keeper, obj, ch, TO_VICT );
        else
            act( AT_TELL, "$n mowi Ci, 'Nie trzeba naprawiac $h!'", keeper, obj, ch, TO_VICT );
    }
    /* "repair all" gets a 10% surcharge - Gorog */

    else if ( (cost = strcmp("all",arg) ? cost : 11*cost/10) > ch->gold )
    {
        sprintf( buf,
                "$N mowi Ci 'Musisz zaplacic %d szt. zlota za %s %s...'", cost,
                fixstr, obj->name );
        act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
        act( AT_TELL, "$N mowi Ci, 'Nie stac Cie na to.'", ch,
                NULL, keeper, TO_CHAR );
    }
    else
    {
        sprintf( buf, "$n daje $v $X, ktory szybko to %s.", fixstr2 );
        act( AT_ACTION, buf, ch, obj, keeper, TO_ROOM );
        sprintf( buf, "$N zabiera Ci %d szt. zlota za %s $f.",
                cost, fixstr );
        act( AT_ACTION, buf, ch, obj, keeper, TO_CHAR );
        ch->gold     -= cost;
        keeper->gold += cost;
        if ( keeper->gold < 0 )
            keeper->gold = 0;
        else
            if ( keeper->gold > maxgold )
            {
                boost_economy( keeper->in_room->area, keeper->gold - maxgold/2 );
                keeper->gold = maxgold/2;
                act( AT_ACTION, "$n chowa czesc zlota do wielkiej kasy.", keeper, 
                        NULL, NULL, TO_ROOM );
            }

        switch ( obj->item_type )
        {
            default:
                send_to_char( "Z dziwnego powodu czujesz, ze cie oszukano...\n\r", ch);
                break;
            case ITEM_ARMOR:
                obj->value[0] = obj->value[1];
                break;
            case ITEM_WEAPON:
                obj->value[0] = INIT_WEAPON_CONDITION;
                break;
            case ITEM_WAND:
            case ITEM_STAFF:
                obj->value[2] = obj->value[1];
                break;
        }

        oprog_repair_trigger( ch, obj );
    }
}

void do_repair( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    char *fixstr;
    char *fixstr2;
    int maxgold;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Co naprawic?\n\r", ch );
        return;
    }

    if ( ( keeper = find_fixer( ch ) ) == NULL )
        return;

    maxgold = keeper->level * keeper->level * 100000;
    switch( keeper->pIndexData->rShop->shop_type )
    {
        default:
        case SHOP_FIX:
            fixstr  = "naprawe";
            fixstr2 = "naprawia";
            break;
        case SHOP_RECHARGE:
            fixstr  = "odnowienie";
            fixstr2 = "odnawia";
            break;
    }

    if ( !strcmp( argument, "all" ) )
    {
        for ( obj = ch->first_carrying; obj ; obj = obj->next_content )
        {
            if ( obj->wear_loc  == WEAR_NONE
                    &&   can_see_obj( ch, obj )
                    &&   can_see_obj( keeper, obj )
                    && ( obj->item_type == ITEM_ARMOR
                        ||   obj->item_type == ITEM_WEAPON
                        ||   obj->item_type == ITEM_WAND
                        ||   obj->item_type == ITEM_STAFF ) )
                repair_one_obj( ch, keeper, obj, argument, maxgold,
                        fixstr, fixstr2);
        }
        return;
    }

    if ( ( obj = get_obj_carry( ch, argument ) ) == NULL )
    {
        act( AT_TELL, "$n mowi Ci 'Nie posiadasz czegos takiego.'",
                keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    repair_one_obj( ch, keeper, obj, argument, maxgold, fixstr, fixstr2); }

void appraise_all( CHAR_DATA *ch, CHAR_DATA *keeper, char *fixstr )
{
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH], *pbuf=buf;
    int cost=0, total=0;

    for ( obj = ch->first_carrying; obj != NULL ; obj = obj->next_content )
    {
        if ( obj->wear_loc  == WEAR_NONE
                &&   can_see_obj( ch, obj )
                && ( obj->item_type == ITEM_ARMOR
                    ||   obj->item_type == ITEM_WEAPON
                    ||   obj->item_type == ITEM_WAND
                    ||   obj->item_type == ITEM_STAFF ) )
        {

            if ( !can_drop_obj( ch, obj ) )
            {
                ch_printf( ch, "Nie mozesz pozbyc sie %s.\n\r", obj->short_descr_dop );
            }
            else if ( ( cost = get_repaircost( keeper, obj ) ) < 0 )
            {
                if (cost != -2)
                    act( AT_TELL,
                            "$n mowi Ci 'Niestety, nie moge nic zrobic z $y.'",
                            keeper, obj, ch, TO_VICT );
                else
                    act( AT_TELL, "$n mowi Ci 'Nie trzeba naprawiac $f!'",
                            keeper, obj, ch, TO_VICT );
            }
            else 
            {
                sprintf( buf,
                        "$N mowi Ci 'Musisz zaplacic %d szt. zlota za %s %s'",
                        cost, fixstr, obj->name );
                act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
                total += cost;
            }
        }
    }
    if ( total > 0 )
    {
        send_to_char ("\n\r", ch);
        sprintf( buf,
                "$N mowi Ci 'To bedzie lacznie kosztowac %d szt. zlota.'",
                total );
        act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
        strcpy( pbuf,
                "$N mowi Ci 'Za naprawe wszystkiego pobieram 10% ceny dodatkowo.'");
        act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
    }
}


void do_appraise( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;
    char *fixstr;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Co chcesz oszacowac?\n\r", ch );
        return;
    }

    if ( ( keeper = find_fixer( ch ) ) == NULL )
        return;

    switch( keeper->pIndexData->rShop->shop_type )
    {
        default:
        case SHOP_FIX:
            fixstr  = "naprawe";
            break;
        case SHOP_RECHARGE:
            fixstr  = "odnowienie";
            break;
    }

    if ( !strcmp( arg, "all") )
    {
        appraise_all( ch, keeper, fixstr );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
        act( AT_TELL, "$n mowi Ci 'Nie posiadasz takiego przedmiotu.'",
                keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
        send_to_char( "Nie mozesz sie tego pozbyc.\n\r", ch );
        return;
    }

    if ( ( cost = get_repaircost( keeper, obj ) ) < 0 )
    {
        if (cost != -2)
            act( AT_TELL, "$n mowi Ci 'Niestety, nie moge nic zrobic z $y.'", keeper, obj, ch, TO_VICT );
        else
            act( AT_TELL, "$n mowi Ci 'Nie trzeba naprawiac $f!'", keeper, obj, ch, TO_VICT );
        return;
    }

    sprintf( buf,
            "$N mowi Ci 'Musisz zaplacic %d szt. zlota za %s tego...'", cost,
            fixstr );
    act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
    if ( cost > ch->gold )
        act( AT_TELL, "$N mowi Ci, 'Jak widze nie stac Cie na to.'", ch,
                NULL, keeper, TO_CHAR );

    return;
}


/* ------------------ Shop Building and Editing Section ----------------- */


void do_makeshop( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA *shop;
    int vnum;
    MOB_INDEX_DATA *mob;

    if ( !argument || argument[0] == '\0' )
    {
        send_to_char( "Usage: makeshop <mobvnum>\n\r", ch );
        return;
    }

    vnum = atoi( argument );

    if ( (mob = get_mob_index(vnum)) == NULL )
    {
        send_to_char( "Mobile not found.\n\r", ch );
        return;
    }

    if ( !can_medit(ch, mob) )
        return;

    if ( mob->pShop )
    {
        send_to_char( "This mobile already has a shop.\n\r", ch );
        return;
    }

    CREATE( shop, SHOP_DATA, 1 );

    LINK( shop, first_shop, last_shop, next, prev );
    shop->keeper	= vnum;
    shop->profit_buy	= 120;
    shop->profit_sell	= 90;
    shop->open_hour	= 0;
    shop->close_hour	= 23;
    mob->pShop		= shop;
    send_to_char( "Zrobione.\n\r", ch );
    return;
}


void do_shopset( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA *shop;
    MOB_INDEX_DATA *mob, *mob2;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int vnum, value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Usage: shopset <mob vnum> <field> value\n\r", ch );
        send_to_char( "\n\rField being one of:\n\r", ch );
        send_to_char( "  buy0 buy1 buy2 buy3 buy4 buy sell open close keeper\n\r", ch );
        return;
    }

    vnum = atoi( arg1 );

    if ( (mob = get_mob_index(vnum)) == NULL )
    {
        send_to_char( "Mobile not found.\n\r", ch );
        return;
    }

    if ( !can_medit(ch, mob) )
        return;

    if ( !mob->pShop )
    {
        send_to_char( "This mobile doesn't keep a shop.\n\r", ch );
        return;
    }
    shop = mob->pShop;
    value = atoi( argument );

    if ( !str_cmp( arg2, "buy0" ) )
    {
        if ( !is_number(argument) )
            value = get_otype(argument);
        if ( value < 0 || value > MAX_ITEM_TYPE )
        {
            send_to_char( "Invalid item type!\n\r", ch );
            return;
        }
        shop->buy_type[0] = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy1" ) )
    {
        if ( !is_number(argument) )
            value = get_otype(argument);
        if ( value < 0 || value > MAX_ITEM_TYPE )
        {
            send_to_char( "Invalid item type!\n\r", ch );
            return;
        }
        shop->buy_type[1] = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy2" ) )
    {
        if ( !is_number(argument) )
            value = get_otype(argument);
        if ( value < 0 || value > MAX_ITEM_TYPE )
        {
            send_to_char( "Invalid item type!\n\r", ch );
            return;
        }
        shop->buy_type[2] = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy3" ) )
    {
        if ( !is_number(argument) )
            value = get_otype(argument);
        if ( value < 0 || value > MAX_ITEM_TYPE )
        {
            send_to_char( "Invalid item type!\n\r", ch );
            return;
        }
        shop->buy_type[3] = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy4" ) )
    {
        if ( !is_number(argument) )
            value = get_otype(argument);
        if ( value < 0 || value > MAX_ITEM_TYPE )
        {
            send_to_char( "Invalid item type!\n\r", ch );
            return;
        }
        shop->buy_type[4] = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy" ) )
    {
        if ( value <= (shop->profit_sell+5) || value > 1000 )
        {
            send_to_char( "Out of range.\n\r", ch );
            return;
        }
        shop->profit_buy = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "sell" ) )
    {
        if ( value < 0 || value >= (shop->profit_buy-5) )
        {
            send_to_char( "Out of range.\n\r", ch );
            return;
        }
        shop->profit_sell = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "open" ) )
    {
        if ( value < 0 || value > 23 )
        {
            send_to_char( "Out of range.\n\r", ch );
            return;
        }
        shop->open_hour = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "close" ) )
    {
        if ( value < 0 || value > 23 )
        {
            send_to_char( "Out of range.\n\r", ch );
            return;
        }
        shop->close_hour = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "keeper" ) )
    {
        if ( (mob2 = get_mob_index(vnum)) == NULL )
        {
            send_to_char( "Mobile not found.\n\r", ch );
            return;
        }
        if ( !can_medit(ch, mob) )
            return;
        if ( mob2->pShop )
        {
            send_to_char( "That mobile already has a shop.\n\r", ch );
            return;
        }
        mob->pShop  = NULL;
        mob2->pShop = shop;
        shop->keeper = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    do_shopset( ch, "" );
    return;
}


void do_shopstat( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA *shop;
    MOB_INDEX_DATA *mob;
    int vnum;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Usage: shopstat <keeper vnum>\n\r", ch );
        return;
    }

    vnum = atoi( argument );

    if ( (mob = get_mob_index(vnum)) == NULL )
    {
        send_to_char( "Mobile not found.\n\r", ch );
        return;
    }

    if ( !mob->pShop )
    {
        send_to_char( "This mobile doesn't keep a shop.\n\r", ch );
        return;
    }
    shop = mob->pShop;

    ch_printf( ch, "Keeper: %d  %s\n\r", shop->keeper, mob->short_descr );
    ch_printf( ch, "buy0 [%s]  buy1 [%s]  buy2 [%s]  buy3 [%s]  buy4 [%s]\n\r",
            o_types[shop->buy_type[0]],
            o_types[shop->buy_type[1]],
            o_types[shop->buy_type[2]],
            o_types[shop->buy_type[3]],
            o_types[shop->buy_type[4]] );
    ch_printf( ch, "Profit:  buy %3d%%  sell %3d%%\n\r",
            shop->profit_buy,
            shop->profit_sell );
    ch_printf( ch, "Hours:   open %2d  close %2d\n\r",
            shop->open_hour,
            shop->close_hour );
    return;
}


void do_shops( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA *shop;

    if ( !first_shop )
    {
        send_to_char( "There are no shops.\n\r", ch );
        return;
    }

    set_char_color( AT_NOTE, ch );
    for ( shop = first_shop; shop; shop = shop->next )
        ch_printf( ch, "Keeper: %5d Buy: %3d Sell: %3d Open: %2d Close: %2d Buy: %2d %2d %2d %2d %2d\n\r",
                shop->keeper,	   shop->profit_buy, shop->profit_sell,
                shop->open_hour,   shop->close_hour,
                shop->buy_type[0], shop->buy_type[1],
                shop->buy_type[2], shop->buy_type[3], shop->buy_type[4] );
    return;
}


/* -------------- Repair Shop Building and Editing Section -------------- */


void do_makerepair( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA *repair;
    int vnum;
    MOB_INDEX_DATA *mob;

    if ( !argument || argument[0] == '\0' )
    {
        send_to_char( "Usage: makerepair <mobvnum>\n\r", ch );
        return;
    }

    vnum = atoi( argument );

    if ( (mob = get_mob_index(vnum)) == NULL )
    {
        send_to_char( "Mobile not found.\n\r", ch );
        return;
    }

    if ( !can_medit(ch, mob) )
        return;

    if ( mob->rShop )
    {
        send_to_char( "This mobile already has a repair shop.\n\r", ch );
        return;
    }

    CREATE( repair, REPAIR_DATA, 1 );

    LINK( repair, first_repair, last_repair, next, prev );
    repair->keeper	= vnum;
    repair->profit_fix	= 100;
    repair->shop_type	= SHOP_FIX;
    repair->open_hour	= 0;
    repair->close_hour	= 23;
    mob->rShop		= repair;
    send_to_char( "Zrobione.\n\r", ch );
    return;
}


void do_repairset( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA *repair;
    MOB_INDEX_DATA *mob, *mob2;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int vnum;
    int value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Usage: repairset <mob vnum> <field> value\n\r", ch );
        send_to_char( "\n\rField being one of:\n\r", ch );
        send_to_char( "  fix0 fix1 fix2 profit type open close keeper\n\r", ch );
        return;
    }

    vnum = atoi( arg1 );

    if ( (mob = get_mob_index(vnum)) == NULL )
    {
        send_to_char( "Mobile not found.\n\r", ch );
        return;
    }

    if ( !can_medit(ch, mob) )
        return;

    if ( !mob->rShop )
    {
        send_to_char( "This mobile doesn't keep a repair shop.\n\r", ch );
        return;
    }
    repair = mob->rShop;
    value = atoi( argument );

    if ( !str_cmp( arg2, "fix0" ) )
    {
        if ( !is_number(argument) )
            value = get_otype(argument);
        if ( value < 0 || value > MAX_ITEM_TYPE )
        {
            send_to_char( "Invalid item type!\n\r", ch );
            return;
        }
        repair->fix_type[0] = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "fix1" ) )
    {
        if ( !is_number(argument) )
            value = get_otype(argument);
        if ( value < 0 || value > MAX_ITEM_TYPE )
        {
            send_to_char( "Invalid item type!\n\r", ch );
            return;
        }
        repair->fix_type[1] = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "fix2" ) )
    {
        if ( !is_number(argument) )
            value = get_otype(argument);
        if ( value < 0 || value > MAX_ITEM_TYPE )
        {
            send_to_char( "Invalid item type!\n\r", ch );
            return;
        }
        repair->fix_type[2] = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "profit" ) )
    {
        if ( value < 1 || value > 1000 )
        {
            send_to_char( "Out of range.\n\r", ch );
            return;
        }
        repair->profit_fix = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "type" ) )
    {
        if ( value < 1 || value > 2 )
        {
            send_to_char( "Out of range.\n\r", ch );
            return;
        }
        repair->shop_type = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "open" ) )
    {
        if ( value < 0 || value > 23 )
        {
            send_to_char( "Out of range.\n\r", ch );
            return;
        }
        repair->open_hour = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "close" ) )
    {
        if ( value < 0 || value > 23 )
        {
            send_to_char( "Out of range.\n\r", ch );
            return;
        }
        repair->close_hour = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "keeper" ) )
    {
        if ( (mob2 = get_mob_index(vnum)) == NULL )
        {
            send_to_char( "Mobile not found.\n\r", ch );
            return;
        }
        if ( !can_medit(ch, mob) )
            return;
        if ( mob2->rShop )
        {
            send_to_char( "That mobile already has a repair shop.\n\r", ch );
            return;
        }
        mob->rShop  = NULL;
        mob2->rShop = repair;
        repair->keeper = value;
        send_to_char( "Zrobione.\n\r", ch );
        return;
    }

    do_repairset( ch, "" );
    return;
}


void do_repairstat( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA *repair;
    MOB_INDEX_DATA *mob;
    int vnum;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Usage: repairstat <keeper vnum>\n\r", ch );
        return;
    }

    vnum = atoi( argument );

    if ( (mob = get_mob_index(vnum)) == NULL )
    {
        send_to_char( "Mobile not found.\n\r", ch );
        return;
    }

    if ( !mob->rShop )
    {
        send_to_char( "This mobile doesn't keep a repair shop.\n\r", ch );
        return;
    }
    repair = mob->rShop;

    ch_printf( ch, "Keeper: %d  %s\n\r", repair->keeper, mob->short_descr );
    ch_printf( ch, "fix0 [%s]  fix1 [%s]  fix2 [%s]\n\r",
            o_types[repair->fix_type[0]],
            o_types[repair->fix_type[1]],
            o_types[repair->fix_type[2]] );
    ch_printf( ch, "Profit: %3d%%  Type: %d\n\r",
            repair->profit_fix,
            repair->shop_type );
    ch_printf( ch, "Hours:   open %2d  close %2d\n\r",
            repair->open_hour,
            repair->close_hour );
    return;
}


void do_repairshops( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA *repair;

    if ( !first_repair )
    {
        send_to_char( "There are no repair shops.\n\r", ch );
        return;
    }

    set_char_color( AT_NOTE, ch );
    for ( repair = first_repair; repair; repair = repair->next )
        ch_printf( ch, "Keeper: %5d Profit: %3d Type: %d Open: %2d Close: %2d Fix: %2d %2d %2d\n\r",
                repair->keeper,	     repair->profit_fix, repair->shop_type,
                repair->open_hour,   repair->close_hour,
                repair->fix_type[0], repair->fix_type[1], repair->fix_type[2] );
    return;
}

CHAR_DATA *find_banker( CHAR_DATA *ch )
{
    CHAR_DATA *banker;

    for ( banker = ch->in_room->first_person; banker; banker = banker->next_in_room )
        if ( IS_NPC( banker ) && IS_BANKER( banker ) )
            break;

    return banker;
}

/* SMAUG Bank Support
 * Coded by Minas Ravenblood for The Apocalypse Theatre
 * (email: krisco7@hotmail.com)
 */
void do_bank( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *banker;
    char arg1[MAX_INPUT_LENGTH];
    char buf [MAX_STRING_LENGTH];
    int amount = 0;

    if ( !( banker = find_banker( ch ) ) )
    {
        send_to_char( "Nie ma tutaj bankiera.\n\r", ch );
        return;
    }

    if ( IS_NPC( ch ) )
    {
        sprintf( buf, "Przepraszam %s, ale nie pracuje z mobami.", ch->short_descr );
        do_say( banker, buf );
        return;
    }

    if ( argument[0] == '\0' )
    {
        do_say( banker, "Jesli czegos nie wiesz, zobacz HELP BANK." );
        return;
    }

    argument = one_argument( argument, arg1 );

    if ( !str_cmp( arg1, "konto" ) )
    {
        int total = ch->pcdata->balance + ch->gold;

        set_char_color( AT_GREEN, ch );
        sprintf( buf, "Masz %d sztuk%s zlota.\n\r", ch->gold, (ch->gold == 1) ? "e" : "" );
        send_to_char( buf, ch );
        sprintf( buf, "Masz takze %d sztuk zlota%s w banku.\n\r",
                ch->pcdata->balance, (ch->pcdata->balance == 1) ? "e" : "" );
        send_to_char( buf, ch );
        sprintf( buf, "Co daje w sumie %d sztuk zlota.\n\r",
                total);
        send_to_char( buf, ch );

        /* Jesli ch jest leaderem lub oficerem klanu,
           podaje stan konta klanowego - Crevan    */

        if ( ch->pcdata->clan && 
                ( !str_cmp( ch->name, ch->pcdata->clan->leader ) 
                  || !str_cmp( ch->name, ch->pcdata->clan->number1 )
                  || !str_cmp( ch->name, ch->pcdata->clan->number2 ) ) )
        {
            sprintf( buf, "Na koncie klanu znajduje sie %ld sztuk zlota.\n\r", ch->pcdata->clan->balance );
            send_to_char( buf, ch );
        }

        /* Crevan - koniec */

        return;
    }

    if ( !str_cmp( arg1, "wplac" ) )
    {
        char arg2[MAX_INPUT_LENGTH];

        argument = one_argument( argument, arg2 );

        if ( arg2 == '\0' )
        {
            sprintf( buf, "%s Ile zlota chcesz wplacic?", ch->name );
            do_tell( banker, buf );
            return;
        }

        if ( str_cmp( arg2, "all" ) && !is_number( arg2 ) )
        {
            sprintf( buf, "%s Ile zlota chcesz wplacic?", ch->name );
            do_tell( banker, buf );
            return;
        }

        if ( !str_cmp( arg2, "all" ) )
            amount = ch->gold;
        else
            amount = atoi( arg2 );

        if ( amount > ch->gold )
        {
            sprintf( buf, "%s Przykro mi, ale nie masz tyle zlota.",
                    ch->name );
            do_tell( banker, buf );
            return;
        }

        if ( amount <= 0 )
        {
            sprintf( buf, "%s Nie robie interesow z zartownisiami.",
                    ch->name );
            do_tell( banker, buf );
            return;
        }

        ch->gold		-= amount;
        ch->pcdata->balance	+= amount;
        sprintf( buf, "Wplacasz %d sztuk%s zlota.\n\r", amount, (amount != 1) ? "" : "e" );
        set_char_color( AT_PLAIN, ch );
        send_to_char( buf, ch );
        sprintf( buf, "$n wplaca %d sztuk%s zlota.\n\r", amount, (amount != 1) ? "" : "e" );
        act( AT_PLAIN, buf, ch, NULL, NULL, TO_ROOM );
        return;
    }

    if ( !str_cmp( arg1, "wyplac" ) )
    {
        char arg2[MAX_INPUT_LENGTH];

        argument = one_argument( argument, arg2 );

        if ( arg2 == '\0' )
        {
            sprintf( buf, "%s Ile zlota chcesz wyplacic?", ch->name );
            do_tell( banker, buf );
            return;
        }
        if ( str_cmp( arg2, "all" ) && !is_number( arg2 ) )
        { 
            sprintf( buf, "%s Ile zlota chcesz wyplacic?", ch->name );
            do_tell( banker, buf );
            return;
        }

        if ( !str_cmp( arg2, "all" ) )
            amount = ch->pcdata->balance;    
        else
            amount = atoi( arg2 );

        if ( amount > ch->pcdata->balance )
        {
            sprintf( buf, "%s Nie masz tyle zlota na koncie!",
                    ch->name );
            do_tell( banker, buf );
            return;
        }

        if ( amount <= 0 )
        {
            sprintf( buf, "%s Nie robie interesow z zartownisiami!.",
                    ch->name );
            do_tell( banker, buf );
            return;
        }

        ch->pcdata->balance	-= amount;
        ch->gold		+= amount;
        sprintf( buf, "Wyplacasz %d sztuk%s zlota.\n\r", amount, (amount != 1) ? "" : "e" );
        set_char_color( AT_PLAIN, ch );
        send_to_char( buf, ch );
        sprintf( buf, "$n wyplaca %d sztuk%s zlota.\n\r", amount, (amount != 1) ? "" : "e" );
        act( AT_PLAIN, buf, ch, NULL, NULL, TO_ROOM );
        return;
    }

    if ( !str_cmp( arg1, "transfer" ) )
    {
        CHAR_DATA *victim;
        CLAN_DATA *clan;
        char arg2[MAX_INPUT_LENGTH];
        char arg3[MAX_INPUT_LENGTH];
        bool clan_account_found;

        argument = one_argument( argument, arg2 );
        argument = one_argument( argument, arg3 );

        if ( arg2 == '\0' || arg3 == '\0' )
        {
            sprintf( buf, "%s Ile zlota chcesz przekazac i komu?", ch->name );
            do_tell( banker, buf );
            return;
        }
        if ( str_cmp( arg2, "all" ) && !is_number( arg2 ) )
        {
            sprintf( buf, "%s Ile zlota chcesz przekazac i komu?", ch->name );
            do_tell( banker, buf );
            return;
        }

        /* Obsluga kont klanowych - Crevan */

        /* Istnieje konto klanowe o nazwie jak arg3 ? */

        clan_account_found = FALSE;
        for ( clan = first_clan; clan; clan = clan->next )
        {
            if ( clan->account != '\0' )
            {
                if ( !str_cmp( arg3, clan->account ) )
                {
                    clan_account_found = TRUE;
                    break;
                }
            }
            else
            {
                bug( "Klan %s nie ma ustawionej nazwy konta.", clan->name );
                bug( "Ustawiam nazwe jak filename: %s.", clan->filename );
                clan->account = STRALLOC( clan->filename );
                save_clan( clan );
            }
        }

        /* Jesli istnieje... */

        if ( clan_account_found )
        {
            if ( !str_cmp( arg2, "all" ) )
                amount = ch->pcdata->balance;
            else
                amount = atoi( arg2 );
            if ( amount <= 0 )
            {
                sprintf( buf, "%s Nie robie interesow z zartownisiami.", ch->name );
                do_tell( banker, buf );
                return;
            }
            if ( amount > ch->pcdata->balance )
            {
                sprintf( buf, "%s Twa szczodrosc mnie zadziwia, ale nia masz tyle zlota!", ch->name );
                do_tell( banker, buf );
                return;
            }

            /* Ktory klan jest wlascicielem konta o nazwie jak arg3 */

            for ( clan = first_clan; clan; clan = clan->next )
            {    
                if ( !str_cmp( clan->account, arg3 ) )
                {
                    sprintf( buf, "Przelewasz %d sztuk%s zlota na konto klanu %s.\n\r",
                            amount, (amount != 1) ? "" : "e", clan->name );
                    set_char_color( AT_GREEN, ch );
                    send_to_char( buf, ch );
                    ch->pcdata->balance -= amount;
                    clan->balance += amount;
                    save_clan( ch->pcdata->clan );

                    /* Informuje leadera, 1 oficera, 2 oficera lub kolejnego czlonka klanu
                       o przelewie - jak nikogo nie ma z klanu - informacja idzie do logow
                       Informacja jest wysylana tylko do pierwszego znalezionego        */

                    if ( ( victim = get_char_world( ch, clan->leader ) ) )
                    {
                        sprintf( buf, "%s przelal wlasnie %d sztuk%s zlota na konto klanu.\n\r",
                                ch->name, amount, (amount != 1) ? "" : "e" );
                        set_char_color( AT_GREEN, victim );
                        send_to_char( buf, victim );
                        return;
                    }
                    if ( ( victim = get_char_world( ch, clan->number1 ) ) )
                    {
                        sprintf( buf, "%s przelal wlasnie %d sztuk%s zlota na konto klanu.\n\r",
                                ch->name, amount, (amount != 1) ? "" : "e" );
                        set_char_color( AT_GREEN, victim );
                        send_to_char( buf, victim );
                        return;
                    }
                    if ( ( victim = get_char_world( ch, clan->number2 ) ) )
                    {
                        sprintf( buf, "%s przelal wlasnie %d sztuk%s zlota na konto klanu.\n\r",
                                ch->name, amount, (amount != 1) ? "" : "e" );
                        set_char_color( AT_GREEN, victim );
                        send_to_char( buf, victim );
                        return;
                    }
                    for ( victim = first_char; victim; victim = victim->next )
                    {
                        if ( IS_NPC( victim ) )
                            continue;
                        if ( victim->pcdata->clan && !str_cmp( clan->name, victim->pcdata->clan->name ) )
                        {
                            sprintf( buf, "%s przelal wlasnie %d sztuk%s zlota na konto klanu.\n\r",
                                    ch->name, amount, (amount != 1) ? "" : "e" );
                            set_char_color( AT_GREEN, victim );
                            send_to_char( buf, victim );
                            return;
                        }
                    }
                    if ( !victim )
                    {
                        sprintf( buf, "%s przelal %d sztuk%s zlota na konto klanu %s.",
                                ch->name, amount, (amount != 1) ? "" : "e", clan->name );
                        log_string_plus( buf, LOG_NORMAL, 0 );
                        to_channel( buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );		    
                        return;
                    }
                }
            }
        }

        /*    if ( !( victim = get_char_world( ch, arg3 ) ) */
        /* Przerobka - nie ma konta klanowego i nie ma takiego gracza polaczonego? */

        victim = get_char_world( ch, arg3 );
        if ( !clan_account_found && !victim )
        {
            sprintf( buf, "%s %s nie ma tu aktualnie konta.", ch->name, capitalize(arg3) );
            do_tell( banker, buf );
            return;
        }

        /* Crevan - koniec kont klanowych uff :) */

        if ( IS_NPC( victim ) )
        {
            sprintf( buf, "%s Nie robie interesow z mobami...", ch->name );
            do_tell( banker, buf );
            return;
        }

        if ( !str_cmp( arg2, "all" ) )
            amount = ch->pcdata->balance;
        else
            amount = atoi( arg2 );

        if ( amount > ch->pcdata->balance )
        {
            sprintf( buf, "%s Twa szczodrosc mnie zadziwia, ale nia masz tyle zlota!", ch->name );
            do_tell( banker, buf );
            return;
        }

        if ( amount <= 0 )
        {
            sprintf( buf, "%s Nie robie interesow z zartownisiami.", ch->name );
            do_tell( banker, buf );
            return;
        }

        ch->pcdata->balance     -= amount;
        victim->pcdata->balance += amount;
        sprintf( buf, "Przelewasz %d sztuk%s zlota na konto %s.\n\r",
                amount, (amount != 1) ? "" : "e", victim->ncel );
        set_char_color( AT_GREEN, ch );
        send_to_char( buf, ch );
        sprintf( buf, "%s wlasnie przelal %d sztuk%s zlota na twoje konto.\n\r",
                ch->name, amount, (amount != 1) ? "" : "e" );
        set_char_color( AT_GREEN, victim );
        send_to_char( buf, victim );
        return;
    }

    if ( !str_cmp( arg1, "help" ) )
    {
        do_help( ch, "bank" );
        return;
    }
    return;
}

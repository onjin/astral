/*
Plik naglowkowy dla modulu kanalow - Crevan
 */
 
typedef struct	channel_data		CHAN_DATA;

#define MAX_CHANNELS		  100

struct channel_data
{
    CHAN_DATA	*next;
    CHAN_DATA	*prev;
    char *	name;			/* nazwa kanalu			*/
    char *	filename;		/* nazwa pliku z danymi		*/
    char *	creator;		/* kto stworzyl kanal		*/
    char *	guild;			/* przynaleznosc kanalu		*/
    int		users;			/* liczba podlaczonych graczy	*/
    sh_int	color;			/* kolorek :)			*/
    sh_int	race_allowed;		/* rasa, -1 jesli ogolny	*/
    sh_int	class_allowed;		/* klasa, -1 jesli ogolny	*/
    sh_int	sex_allowed;		/* plec, -1 jesli ogolny	*/
    sh_int	min_talk;		/* min. level do uzywania	*/
    sh_int	is_logged;		/* logowany?			*/
    sh_int	is_private;		/* prywatny, na razie nie ma	*/
};					/* pelnej obslugi		*/

DECLARE_DO_FUN( do_kanaly	);
DECLARE_DO_FUN( do_setchannel	);
DECLARE_DO_FUN( do_showchannel	);
DECLARE_DO_FUN(	do_makechannel	);
DECLARE_DO_FUN( do_update_channels );

#define CHANNEL_DIR	"../channels/"
#define CHANNEL_LIST	"channel.lst"

CHAN_DATA * get_channel	args( ( char *name ) );
void	load_channels	args( ( void ) );
void	save_channel	args( ( CHAN_DATA *chan ) );
void	correct_channels args( ( void ) );
void	check_char_channels args( ( CHAR_DATA *ch ) );
bool	check_virtual_channels args( ( CHAR_DATA *ch, char *command, char *message ) );


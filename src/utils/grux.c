/*
 * Grub Extract - creates an extract file for the GRUB command
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define  DIR_NAME "/home/smaug/player/"	/* mud home directory here! */
#define  MAX_STRING_LENGTH 4096
#define  MAX_NAME_LENGTH 13
#define  MAX_SITE_LENGTH 16
#define  PCFLAG_DEADLY    2
#define  LOWER(c)        ((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))

time_t	now_time;
int	deleted = 0;
int	output = 0;

struct rec_struct
{
   char    name [MAX_NAME_LENGTH];
   char    sex;
   char    class;
   char    race;
   char    level;
   short   room;
   long    gold;
   char    clan;
   char    council;
   char    site [MAX_SITE_LENGTH];
   long    last;
   char    pkill;
};

char *get_arg( char *argument, char *arg_first )
{
    int count=0;

    while ( isspace(*argument) ) argument++;
    while ( *argument != '\0' && *argument != 10 && ++count <= 255 )
    {
       if ( *argument == ' ' ) {argument++; break;}
       *arg_first = LOWER(*argument);
       arg_first++;
       argument++;
    }
    *arg_first = '\0';
    while ( isspace(*argument) ) argument++;
    return argument;
}

void read_pfile (char *dirname, char *filename, FILE *ofp)
{
  FILE *fp;
  struct rec_struct r;
  char b[MAX_STRING_LENGTH], s[MAX_STRING_LENGTH], *ps;
  char fname[MAX_STRING_LENGTH];
  struct stat fst;
  struct tm *t;
  time_t tdiff;
  int    flags;

  sprintf( fname, "%s/%s", dirname, filename );

  if ( stat( fname, &fst ) != -1 )
  {
     t = localtime( &fst.st_mtime );
  }
  else
  {
     t = NULL;
  }

  if ( ( fp = fopen( fname, "r" ) ) == NULL )
     return;
  memset( &r, 0, sizeof r);
  fgets( s, 2048, fp);
  while (!feof(fp))
  {
     ps = s;
     if (ferror(fp)) {printf("file error\n"); break;}

     if ( s[0]=='N' && s[1]=='a' && s[2]=='m' && s[3]=='e' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        if ( b[ strlen(b) - 1] == '~' ) b[ strlen(b) - 1] = '\0';
        strcpy (r.name, b);
     }
     else if ( s[0]=='S' && s[1]=='e' && s[2]=='x' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        r.sex = atoi(b);
     }
     else if ( s[0]=='C' && s[1]=='l' && s[2]=='a' && s[3]=='s' && s[4]=='s' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        r.class = atoi(b);
     }
     else if ( s[0]=='R' && s[1]=='a' && s[2]=='c' && s[3]=='e' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        r.race = atoi(b);
     }
     else if ( s[0]=='L' && s[1]=='e' && s[2]=='v' && s[3]=='e' && s[4]=='l' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        r.level = atoi(b);
     }
     else if ( s[0]=='R' && s[1]=='o' && s[2]=='o' && s[3]=='m' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        r.room = atoi(b);
     }
     else if ( s[0]=='G' && s[1]=='o' && s[2]=='l' && s[3]=='d' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        r.gold = atol(b);
     }
     else if ( s[0]=='C' && s[1]=='l' && s[2]=='a' && s[3]=='n' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        if ( b[ strlen(b) - 1] == '~' ) b[ strlen(b) - 1] = '\0';
             if ( !strcmp( b, "guild") ) r.clan=1;
        else if ( !strcmp( b, "dragonslayer") ) r.clan=2;
        else if ( !strcmp( b, "maidenstone") ) r.clan=3;
        else if ( !strcmp( b, "ringbearers") ) r.clan=4;
        else if ( !strcmp( b, "arcanes") ) r.clan=5;
        else if ( !strcmp( b, "brujah") ) r.clan=6;
        else if ( !strcmp( b, "lasombra") ) r.clan=7;
        else if ( !strcmp( b, "nosferatu") ) r.clan=8;
        else if ( !strcmp( b, "tremere") ) r.clan=9;
        else if ( !strcmp( b, "ventrue") ) r.clan=10;
        else if ( !strcmp( b, "inconnu") ) r.clan=11;
     }
     else if ( s[0]=='C' && s[1]=='o' && s[2]=='u' && s[3]=='n' && s[4]=='c'
     &&        s[5]=='i' && s[6]=='l' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        if ( b[ strlen(b) - 1] == '~' ) b[ strlen(b) - 1] = '\0';
             if ( !strcmp( b, "council") ) r.council=1;
        else if ( !strcmp( b, "mortal") ) r.council=2;
        else if ( !strcmp( b, "newbie") ) r.council=3;
        else if ( !strcmp( b, "project") ) r.council=4;
        else if ( !strcmp( b, "pkill") ) r.council=5;
        else if ( !strcmp( b, "quest") ) r.council=6;
        else if ( !strcmp( b, "neophyte") ) r.council=7;
        else if ( !strcmp( b, "code") ) r.council=8;
        else if ( !strcmp( b, "area") ) r.council=9;
     }
     else if ( s[0]=='F' && s[1]=='l' && s[2]=='a' && s[3]=='g' && s[4]=='s' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        flags = atoi(b);
        r.pkill = (PCFLAG_DEADLY == (flags & PCFLAG_DEADLY)) ? 'y' : 'n';
     }
     else if ( s[0]=='S' && s[1]=='i' && s[2]=='t' && s[3]=='e' )
     {
        ps = get_arg (ps, b); ps = get_arg (ps, b);
        if ( !isdigit(b[0]) )
           strcpy(r.site, "(unknown)");
        else
           strcpy(r.site, b);
        break;
     }
     fgets(s, 2048, fp);
  }
  fclose (fp);
  r.last = 10000 * t->tm_year + 100 * (t->tm_mon+1) + t->tm_mday;
/*
  printf(
     "%-12s %1u %1u %2u %2u %5u %10ld %3u %3u %-15s %6ld %c\n\r",
     r.name, (unsigned char) r.sex, (unsigned char) r.class,
     (unsigned char) r.race, (unsigned char) r.level, r.room, r.gold,
     (unsigned char) r.clan, (unsigned char) r.council, r.site,
     r.last, r.pkill);
*/
  tdiff = (now_time - fst.st_mtime) / 86400;
  if ( (r.level < 3 && tdiff > 7)
  ||   (r.level < 4 && tdiff > 14)
  ||   (r.level < 5 && tdiff > 21)
  ||   (tdiff > 93) )
  {
     if ( unlink(fname) == -1 )
	perror( "Unlink" );
     else
     {
	++deleted;
        return;
     }
  }

  if (fwrite(&r, sizeof r, 1, ofp) < 1)
  {
     printf("write error");
     exit(1);
  }
  output++;
}

void main (void)
{
  FILE *ofp;
  DIR *dp;
  struct dirent *dentry;
  char dir_name[100];
  int alpha_loop;
  int cou=0;

  now_time = time(0);
  nice(20);

  if ( ( ofp = fopen( "/home/mud/grub.dat", "w" ) ) == NULL )
  {
     printf("Open error on output file.\n\r");
     exit(1);
  }

  for (alpha_loop=0; alpha_loop<=25; alpha_loop++)
  {
      sprintf (dir_name, "%s%c", DIR_NAME, 'a' + alpha_loop);
      printf ("dir=%s\n\r", dir_name);
      dp = opendir( dir_name );
      dentry = readdir( dp );
      while ( dentry )
      {
         if ( dentry->d_name[0] != '.' )
            {
            read_pfile (dir_name, dentry->d_name, ofp);
            cou++;
            }
         dentry = readdir( dp );
      }
      closedir( dp );
      printf ("Count=%d, deleted %d output %d\n\r", cou, deleted, output);
  }
  fclose(ofp);
}

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#define DATAFILE "/var/chance.data" //File to Store User Data

//Custom User Struct to Store information about Users

struct user{
	int uid;
	int credits;
	int highscore;
	char name[100];
	int (* current_game) ();
};

//Function Prototypes

int get_player_data();
void register_new_player();
void update_player_data();
void show_highscore();
void jackpot();
void input_name();
void print_cards(char *, char *, int);
int take_wager(int, int);
void play_the_game();
int pick_a_number();
int dealer_no_match();
int find_the_ace();
void fatal(char *);

//Global Variables

struct user player;	//Player Struct

int main()
{
	int choice, last_game;

	srand(time(0)); //Seed the randomizer with current time.

	if(get_player_data() == -1)
		register_new_player();

	while(choice != 7)
	{
		printf("-=[ Game of Chance Menu ]=-\n");
		printf("1 - Play  Pick a Number Game\n");
		printf("2 - Play  No Match Dealer Game\n");
		printf("3 - Play  Find The Ace Game\n");
		printf("4 - View Current High Score\n");
		printf("5 - Change Your User Name\n");
		printf("6 - Reset Your Account At 100 Credits\n");
		printf("7 - Quit\n");

		printf("\n[Name: %s]\n", player.name);
		printf("[You have %u Credits] -> ", player.credits);
		scanf("%d", &choice);

		if((choice < 1) || (choice > 7))
			printf("\n[!!] The Number %d is an invalid selection.\n\n",choice);
		else if(choice < 4) //Otherwise, Choice was a game of some sort.
		{
			if(choice != last_game) //If the function ptr isn't set
			{
				if(choice == 1)
					player.current_game = &pick_a_number;
				else if(choice == 2)
					player.current_game = &dealer_no_match;
				else
					player.current_game = &find_the_ace;

				last_game = choice;
			}
			play_the_game();
		}
		else if(choice == 4)
			show_highscore();
		else if(choice == 5)
		{
			printf("\nChange user name\n");
			printf("Enter Your New Name: ");
			input_name();
			printf("Your name has been changed.\n\n");
		}
		else if(choice == 6)
		{
			printf("Your account has been reset with 100 credits.\n\n");
			player.credits = 100;
		}
	}
	update_player_data();
	printf("\nThanks for playing!. Bye\n");
}

//This Function Reads The Player Data For UID From The File.
//It Return -1 If It Unable To Find The Player
//Data For Current UID.

int get_player_data()
{
	int fd, uid, read_bytes;
	struct user entry;

	uid = getuid();

	fd = open(DATAFILE, O_RDONLY);
	if(fd == -1)
		return -1;
	read_bytes = read(fd, &entry, sizeof(struct user));	//Read The First Chunk
	while(entry.uid != uid && read_bytes > 0)	//Loop Until Proper UID is Found
	{
		read_bytes = read(fd, &entry, sizeof(struct user));	//Keep Reading
	}
	close(fd);	//Close The File
	if(read_bytes < sizeof(struct user))	//This Means That The End Of The File Was Reached
		return -1;
	else
		player = entry;	//Copy The Read Entry Into Player Struct.
	return 1;	//Return Success
}

//This Is The New User Registration Function.
//It Will Create A New Player Accout And Append It To The File

void register_new_player()
{
	int fd;

	printf("-=-={ New Player Registration }=-=-\n");
	printf("Enter Your Name: ");
	input_name();

	player.uid = getuid();
	player.highscore = player.credits = 100;

	fd = open(DATAFILE, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
	if(fd == -1)
		fatal("in register_new_player() while opening file");
	write(fd, &player, sizeof(struct user));
	close(fd);

	printf("\nWelcome To The Game Of Chance %s.\n", player.name);
	printf("\nYou have been given %u credits.\n", player.credits);
}

//This Function Writes Current Player Data To FIle
//It Is Used For Updating The Credits After Games

void update_player_data()
{
	int fd, i, read_uid;
	char burned_byte;

	fd = open(DATAFILE, O_RDWR);
	if(fd == -1)
		fatal("in update_player_data() while opening file");
	read(fd, &read_uid, 4);	//Read UID From First Struct
	while(read_uid != player.uid)	//Loop Until Correct UID is Found
	{
		for(i = 0; i < sizeof(struct user) - 4; i++)
			read(fd, &burned_byte, 1);	//Read Through The Rest Of That Struct
		read(fd, &read_uid, 4);	//Read The UID From Next Struct
	}
	write(fd , &(player.credits),4);	//Update Credits
	write(fd , &(player.highscore),4);	//Update HighScore
	write(fd , &(player.name),100);		//Update Name
	close(fd);
}

//This Function Will Display The Current HighScore And The Name Of The Person Who Set That HighScore

void show_highscore()
{
	unsigned int top_score = 0;
	char top_name[100];
	struct user entry;

	int fd;

	printf("\n===============| HIGH SCORE |===============\n");
	fd = open(DATAFILE, O_RDONLY);
	if(fd == -1)
		fatal("in show_highscore() while opening file");
	while(read(fd, &entry, sizeof(struct user)) > 0)	//Loop Until End Of The File
	{
		if(entry.highscore > top_score)	//If There Is A Higher Score,
		{
			top_score = entry.highscore; 	//Set Top Score To That Score
			strcpy(top_name, entry.name);	//Set Top Name To That Name
		}
	}
	close(fd);
	if(top_score > player.highscore)
		printf("%s Has The HighScore of %u\n", top_name, top_score);
	else
		printf("You Currently Have The HighScore of %u Credits!\n",player.highscore);
	printf("=================================================\n\n");
}

//This Function Simply Awards The Jackpot For The Pick A Number Game.

void jackpot()
{
	printf("*+*+*+*+*+ JACKPOT *+*+*+*+*+\n");
	printf("You Have Won The Jackpot of 100 Credits!\n");
	player.credits += 100;
}

//This Function Is Used To Input The Player Name, Because scanf() will stop input after first space

void input_name()
{
	char *name_ptr, input_char = '\n';
	while(input_char == '\n')	//Flush Any LeftOver
		scanf("%c", &input_char);	//NewLine Chars

	name_ptr = (char *)&(player.name); 	//name_ptr = player name address
	while(input_char != '\n')	//Loop Until NewLine
	{
		*name_ptr = input_char;	//Put The input char into name field
		scanf("%c", &input_char); //Get The Next Char.
		name_ptr++;
	}
	*name_ptr = 0; 	//Terminate The String
}

//This Function Prints The 3 Cards For The Find The Ace Game.
//It Expects a message to display, a pointer to the cards array,
//and the card the user has picked as input. if the user_pick is -1
//then the selection numbers are displayed.

void print_cards(char *message, char *cards, int user_pick)
{
	int i;

	printf("\n\t*** %s ***\n",message);
	printf("      \t._.\t._.\t._.\n");
	printf("Cards:\t|%c|\t|%c|\t|%c|\n\t", cards[0], cards[1], cards[2]);
	if(user_pick == -1)
		printf(" 1 \t 2 \t 3\n");
	else{
		for(i = 0; i < user_pick; i++)
			printf("\t");
		printf(" ^-- Your Pick\n");
	}
}

//This function inputs wagers for both the No Match Delaer and 
//Find The Ace Games. It expects the available credits and the
//previous wager as arguments. the previous_wager is only important
//for the second wager in the Find The Ace Game. The Function
//return -1 If The Wager is Too Big or Too Little, and it returns the wager amount otherwise.

int take_wager(int available_credits, int previous_wager)
{
	int wager, total_wager;

	printf("How Many of your %d credits would you like to wager? ", available_credits);
	scanf("%d", &wager);
	if(wager < 1)
	{
		printf("Nice Try, But You Must Wager A Positive Number!\n");
		return -1;
	}

	total_wager = previous_wager + wager;
	if(total_wager > available_credits)
	{
		printf("Your Total Wager of %d is More Than You Have!\n", total_wager);
		printf("You Only Have %d Available Credits, Try Again.\n", available_credits);
		return -1;
	}
	return wager;
}

//This Function Contains a Loop to allow the current game to be played again
//it also writes the new credit totals to file after each game played.

void play_the_game()
{
	int play_again = -1;
	int (*game) ();
	char selection;

	while(play_again)
	{
		printf("\n[DEBUG] current_game pointer @ %p\n", player.current_game);
		if(player.current_game() != -1)	//If The Game Plays Without Error and 
		{
			if(player.credits > player.highscore) 	// a new high score is set
				player.highscore = player.credits; //Update the highscore
			printf("\nYou now have %u credits\n", player.credits);
			update_player_data();			//Write the new credit total to file
			printf("Would You Like to Play Again? (Y/N) ");
			selection = '\n';
			while(selection == '\n')		//Flush Any Extra NewLines
				scanf("%c",&selection);
			if(selection == ('n' || 'N'))
				play_again = 0;
		}
		else					//This Means The Game Returned an Error
			play_again = 0;
	}
}

//This Function is The Pick a Number Game
//It Returns -1 If The Player Doesn't Have Enough Credits

int pick_a_number()
{
	int pick, winning_number;

	printf("\n########## PICK A NUMBER ##########\n");
	printf("This Game Costs 10 Credits To Play, Simply Pick A Number\n");
	printf("Between 1 and 20, And If You Pick The Winning Number, You\n");
	printf("will win the jackpot of 100 credits!\n\n");
	winning_number = (rand() % 20) + 1; 	//Pick a Number between 1 and 20
	if(player.credits < 10)
	{
		printf("You Only Have %u Credits. That's Not Enough To Play!\n\n",player.credits);
		return -1;
	}
	player.credits -= 10;
	printf("10 Credits Have Been deducted from Your Account.\n");
	printf("Pick A Number Between 1 and 20: ");
	scanf("%d", &pick);

	printf("The Winning Number is %d\n",winning_number);
	if(pick == winning_number)
		jackpot();
	else
		printf("Sorry, You Didn't Win.\n");
	return 0;
}

//This Function is The No Dealer Match Game
//It Returns -1 if The Player Has 0 Credits.

int dealer_no_match()
{
	int i, j, numbers[16], wager = -1, match = -1;

	printf("\n::::::::::::::: NO MATCH DEALER :::::::::::::::\n");
	printf("In This Game, You Can Wager Up To All Of Your Credits.\n");
	printf("The Dealer Will Deal Out 16 Random Numbers Between 0 to 99.\n");
	printf("If There Are No Matches Among Them, You Double Your Money!\n\n");

	if(player.credits == 0)
	{
		printf("You Don't Have Any Credits To Wager!\n\n");
		return -1;
	}
	while(wager == -1)
		wager = take_wager(player.credits, 0);

	printf("\t\t:::: Dealing Out 16 Random Numbers ::::\n");

	for(i = 0; i < 16; i++)
	{
		numbers[i] = rand() % 100;
		printf("%2d\t", numbers[i]);
		if(i%8 == 7)		//Prinf a Line, Break Every 8 Numbers
			printf("\n");
	}

	for(i = 0; i < 15; i++)		//Loop Looking For Matches
	{
		j = i + 1;
		while(j < 16)
		{
			if(numbers[i] == numbers[j])
				match = numbers[i];
			j++;
		}
	}

	if(match != -1)
	{
		printf("The Dealer Matched The Number %d!\n", match);
		printf("You Lose %d Credits.\n", wager);
		player.credits -= wager;
	}
	else
	{
		printf("There were no matches! You Win %d Credits!\n",wager);
		player.credits += wager;
	}

	return 0;
}

//This Function is The Find The Ace Game

int find_the_ace()
{
	int i, ace, total_wager;
	int invalid_choice, pick = -1, wager_one = -1, wager_two = -1;
	char choice_two, cards[3] = {'X', 'X', 'X'};

	ace = rand() % 3; //Place The Ace Randomly

	printf("\n*************** FIND THE ACE ***************\n");
	printf("In This Game, You Can Wager Up To All Of Your Credits.\n");
	printf("Three Cards Will Be Dealt Out, Two Queens And One Ace.\n");
	printf("If You Find The Ace, You Will Win Your Wager.\n");
	printf("After Choosing A Card,One Of The Queens Will Be Revealed.\n");
	printf("At This Point, You May Either Select A Different Card Or\n");
	printf("Increase Your Wager.\n\n");

	if(player.credits == 0)
	{
		printf("You Don't Have Any Credits To Wager!\n\n");
		return -1;
	}

	while(wager_one == -1)
		wager_one = take_wager(player.credits, 0);

	print_cards("Dealing Cards",cards, -1);
	pick = -1;

	while((pick < 1) || (pick > 3))
	{
		printf("Select a Card: 1, 2 or 3 ");
		scanf("%d", &pick);
	}

	pick--;
	i = 0;

	while(i == ace || i == pick) 	//Keep Looping Until We Find A Valid Queen To Reveal
		i++;
	cards[i] = 'Q';
	print_cards("Revealing a Queen", cards, pick);

	invalid_choice = 1;
	while(invalid_choice) 		//Loop Until a Valid Choice is made
	{
		printf("Would You Like To:\n[C]hange Your Pick\tOr\t[i]ncrease Your Wager?\n");
		printf("Select c or i: ");
		choice_two = '\n';
		while(choice_two == '\n')
			scanf("%c", &choice_two);
		if(choice_two == ('i' || 'I'))
		{
			invalid_choice = 0;
			while(wager_two == -1)
				wager_two = take_wager(player.credits, wager_one);
		}

		if(choice_two == ('c' || 'C'))
		{
			i = invalid_choice = 0;
			while(i == pick || cards[i] == 'Q') 	//Loop Until a Second Card is Found and the swap
				i++;
			pick = i;
			printf("Your Card Pick Has Been Changed To Card %d\n",pick+1);
		}
	}

	for(i = 0; i < 3; i++)
	{
		if(ace == i)
			cards[i] = 'A';
		else
			cards[i] = 'Q';
	}

	print_cards("End Result", cards, pick);

	if(pick == ace) // Handle Win
	{
		printf("You Have Won %d Credits From Your First Wager\n", wager_one);
		player.credits += wager_one;
		if(wager_two != -1)
		{
			printf("And An Additional %d Credits From Your Second Wager!\n",wager_two);
			player.credits += wager_two;
		}
	}
	else 		// Handle Loss
	{
		printf("You Have Lost %d Credits From Your First Wager\n",wager_one);
		player.credits -= wager_one;
		if(wager_two != -1)
		{
			printf("And An Additional %d Credits From Your Second Wager!\n",wager_two);
			player.credits -= wager_two;
		}
	}
	return 0;
}

//This Function is To Display Fatal Errors

void fatal(char *message)
{
	char error_message[100];

	strcpy(error_message,"[!!] Fatal Error ");
	strncat(error_message,message,83);
	perror(error_message);
	exit(-1);
}

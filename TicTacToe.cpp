#include <list>
#include <map>
#include <iostream>
#include <algorithm>
#include <stdlib.h> 
#include <time.h>

using namespace std;
//constants
const int MAX_WINNING = 1;
const int MIN_WINNING = -1;
const int EQUAL_GAME = 0;
const int MAXIMUM_SQUARES = 9;
bool alpha_beta_on = false; //abp toggle
static int counter = 0;

void print_directions()
{
	cout << "This 3x3 TicTacToe game allows you to play" << endl << "against another human player or against the machine." << endl << "You will be prompted for these choices." << endl << "Should you choose to play against a machine, you will be prompted wheather you want the machine to go first." << endl << "Also, you will be prompted whether alpha - beta should be used." << endl;
	cout << endl;
	cout << "The TicTacToe will be indiex starting at 0 through 8 in row major form." << endl;
	cout << "0 1 2\n";
	cout << "3 4 5\n";
	cout << "6 7 8\n";
	cout << "A human player will be prompted for the square number between 0-8." << endl << "A human player will be prompted again if they enter a square number out of range" << endl << "or already filled." << endl;
	return;
}


bool optionPrompt(const char *p)
{
	char option;
	cout << p << "? ";
	cin >> option;
	if (option == 'Y' || option == 'y') {
		return true;
	}
	else {
		return false;
	}
}


//How to represent a gameState?
// Squares are represented with the following indices
/*
0 1 2
3 4 5
6 7 8

Could represent with and X "board" and a separate O "board"
and use bitstrings for each board: 1 if a square is occupied, 0 if not.

layout bits
8 7 6 5 4 3 2 1 0

| X |
X | X | O
| O |
Would be represented as 8 7 6 5 4 3 2 1 0
gameState.x: 0 0 0 0 1 1 0 1 0
gameState.o: 0 1 0 1 0 0 0 0 0
.x | .o   =  0 1 0 1 1 1 0 1 0
1 represented as
0 0 0 0 0 0 0 ... 0 0 0 0 1
1 << 2
0 0 0 0 ...0 0 0 0 1 0 0

((.x | .o)  & (1 << 2)) =   0 1 0 1 1 1 0 1 0
&    0 0 0 0 0 0 1 0 0
0 0 0 0 0 0 0 0 0

!((.x | .o)  & (1 << 1))  is 0
O | X | O
X | X | O
X | O | X
Would be represented as 8 7 6 5 4 3 2 1 0
gameState.x: 1 0 1 0 1 1 0 1 0
gameState.o: 0 1 0 1 0 0 1 0 1
.x | .o   =  1 1 1 1 1 1 1 1 1
0x1FF


*/

// Helper functions to manipulate the bitboards
inline bool TOP_ROW(short x) //z is either x or o 
{
	return (x & 7) == 7;
}


inline bool MID_ROW(short x)
{
	return (x & 0x38) == 0x38;
}


inline bool BOT_ROW(short x)
{
	return (x & 0x1C0) == 0x1C0;
}


inline bool LEFT_COL(short x)
{
	return (x & 0x49) == 0x49;
}


inline bool MID_COL(short x)
{
	return (x & 0x92) == 0x92;
}

inline bool RIGHT_COL(short x)
{
	return (x & 0x124) == 0x124;
}

inline bool DiagNW_SE(short x)
{
	return (x & 0x111) == 0x111;
}

inline bool DiagNE_SW(short x)
{
	return (x & 0x54) == 0x54;
}
typedef int utility;

class action {
public:
	action() {
		a = 0;
		side = 0;
	}
	action(short aParm, short sideParm) {
		a = aParm;
		side = sideParm;
	}
	enum { MIN = 1, MAX = 2 };
	short a;
	short side;
};


class gameState {
public:
	short x;
	short o;

	gameState() { //blank tictactoe grid
		x = 0;
		o = 0;
	}


	// == and < comparison functions needed for comparisons in map
	bool operator==(const gameState& g) const
	{
		return (x == g.x) && (o == g.o);
	}
	bool operator<(const gameState& g) const
	{
		if (x < g.x) return true;
		if (x == g.x && o < g.o) return true;
		else return false;
	}

	inline bool isFilled() const {
		return (x | o) == 0x1FF;
	}

	inline bool win(short s) const {
		return TOP_ROW(s) ||
			MID_ROW(s) ||
			BOT_ROW(s) ||
			LEFT_COL(s) ||
			MID_COL(s) ||
			RIGHT_COL(s) ||
			DiagNW_SE(s) ||
			DiagNE_SW(s);
	}

	inline bool minWin() const {
		return win(o);
	}

	inline bool maxWin() const {
		return win(x);
	}
	//prints xs and os
	inline char print_one(int index) const
	{
		if ((x & index) != 0) return 'x';
		else if (o & index) return 'o';
		else return '_';
	}
	//another print function
	friend ostream& operator<<(ostream& s, gameState& gs) {
		s << gs.print_one(1) << " " << gs.print_one(2) << " " << gs.print_one(4) << endl;
		s << gs.print_one(8) << " " << gs.print_one(16) << " " << gs.print_one(32) << endl;
		s << gs.print_one(64) << " " << gs.print_one(128) << " " << gs.print_one(256) << endl;
		return s;
	}
};

bool end_state(const gameState& gs)
{
	if (gs.maxWin() || gs.minWin() || gs.isFilled()) return true;

	return false;
}

//evaluation the state of the board with MAX being the X player and min being the O player
void eval(const gameState& gs, utility& val)
{
	if (gs.maxWin()) val = MAX_WINNING;
	else if (gs.minWin()) val = MIN_WINNING;
	else if (gs.isFilled()) val = EQUAL_GAME;
}

//creates a list of candidate moves
void getCandidates(const gameState& gs, list<action>& candidates, int side)
{
	for (short i = 0; i < MAXIMUM_SQUARES; i++) {
		if (!((gs.x | gs.o) & (1 << i))) {
			action a;
			a.a = (1 << i);
			a.side = side;
			candidates.push_back(a);
		}
	}
}


short getValidChoice(const gameState &gs, const char *p)
{
	//Check index for valid range 0-8
	//check for unoccupied square
	short index;
	while (true) {
		cout << p;
		cin >> index;
		if (cin.good()) {
			if (index >= 0 && (index <= 8))
				if (!((gs.x | gs.o) & (1 << index)))
					break;
		}
		else {
			cin.clear();
			cin.ignore(INT_MAX, '\n');
		}
		cout << "Invalid input, expected [0-8]" << endl;
	}
	return index;
}

//applies the input to the board
gameState applyCandidate(const gameState &gs, const action &act)
{
	gameState next_gs;
	if (act.side == action::MIN) {
		next_gs.x = gs.x;
		next_gs.o = gs.o | act.a;
	}
	else {
		if (act.side == action::MAX) {
			next_gs.o = gs.o;
			next_gs.x = gs.x | act.a;
		}
	}
	return next_gs;
}

// The transposition table class. Wrapper for the STL map
class ttable {
	map<gameState, utility> tbl;
	bool on;
public:

	ttable() {
		on = false;
	}
	~ttable() {
		tbl.erase(tbl.begin(), tbl.end());
	}


	void toggle(bool on) {
		on = on;
	}

	bool get(const gameState& gs, utility& u) {
		if (!on) return false;

		if (tbl.find(gs) == tbl.end()) return false;
		u = tbl[gs];
		return true;
	}
	void put(const gameState& gs, const utility& u) {
		if (!on) return;

		tbl[gs] = u;
	}
};

static ttable trans_tbl;

void minimax(const gameState&, action&, utility&);
void maxVal(const gameState&, utility, utility, action&, utility&);
void minVal(const gameState&, utility, utility, action&, utility&);


void minimax(const gameState& gs, action& act, utility& val)
{
	utility alpha = -INT_MAX;
	utility beta = INT_MAX;
	maxVal(gs, alpha, beta, act, val);
}

//Find best action from MAX (X) perspective
void maxVal(const gameState& gs, utility alpha, utility beta,
	action& act, utility& val)
{
	utility best_util = -INT_MAX;
	action best_action;

	// If we reached the end state, get the evalutation
	// of the position
	if (end_state(gs)) {
		eval(gs, val);
		return;
	}

	// Get a list of candidate moves
	list<action> candidates;
	getCandidates(gs, candidates, action::MAX);

	// Iterate over all candidates
	list<action>::iterator end = candidates.end();
	for (list<action>::iterator itr = candidates.begin();
		itr != end; itr++) {

		// Apply this candidate move, to get the new
		// game state
		gameState next_gs = applyCandidate(gs, *itr);

		utility next_util;
		action next_action;

		// Do we have this game_state already in the transposition
		// table?
		if (!trans_tbl.get(next_gs, next_util)) {

			// No, keep searching to get evaluation
			minVal(next_gs, alpha, beta, next_action, next_util);

			// Save this evaluation for later
			trans_tbl.put(next_gs, next_util);
		}
		// If this utility value is better than the best
		// utility found so far, save it, and also save the 
		// action that produced this utility
		if (next_util > best_util) {
			best_util = next_util;
			best_action = *itr;
		}

		if (alpha_beta_on) {
			// Did we hit a beta cutoff?
			if (next_util >= beta) {
				break;
			}
			else {
				// Update alpha value
				alpha = max(alpha, next_util);
			}
		}
	}

	// Assign output parameters
	act = best_action;
	val = best_util;

	// Increment trace counter
	counter++;
}

//Find best action from MIN (O) perspective
void minVal(const gameState& gs, utility alpha, utility beta,
	action& act, utility& val)
{
	utility best_util = INT_MAX;
	action best_action;

	// If we reached the end state, get the evalutation
	// of the position
	if (end_state(gs)) {
		eval(gs, val);
		return;
	}

	// Get a list of candidate moves
	list<action> candidates;
	getCandidates(gs, candidates, action::MIN);

	// Iterate over all candidates
	list<action>::iterator end = candidates.end();
	for (list<action>::iterator itr = candidates.begin();
		itr != end; itr++) {
		// Apply this candidate move, to get the new
		// game state
		gameState next_gs = applyCandidate(gs, *itr);

		utility next_util;
		action next_action;
		if (!trans_tbl.get(next_gs, next_util)) {
			maxVal(next_gs, alpha, beta, next_action, next_util);

			trans_tbl.put(gs, next_util);
		}
		if (next_util < best_util) {
			best_util = next_util;
			best_action = *itr;
		}

		if (alpha_beta_on) {
			if (next_util <= alpha) {
				break;
			}
			else {
				beta = min(beta, next_util);
			}
		}
	}
	act = best_action;
	// Assign output parameters
	val = best_util;

	// Increment trace counter
	counter++;
}

int main()
{
	utility val;
	action act;
	gameState gs;
	/*bool done = false;*/
	bool twoHumans;
	bool cpu;

	print_directions();
	twoHumans = optionPrompt("Two humans (y/n)");
	if (twoHumans) { //start alternating between X and O starting with X
		while (true) {
			action x;
			x.a = 1 << getValidChoice(gs, "Player X choice? ");
			x.side = action::MAX;
			gs = applyCandidate(gs, x);
			cout << gs << endl;
			if (gs.maxWin()) {
				cout << "Player X wins!" << endl;
				break;
			}
			if (gs.isFilled()) {
				cout << "Cat's game!" << endl;
				break;
			}
			action o;
			o.a = 1 << getValidChoice(gs, "Player O choice? ");
			o.side = action::MIN;
			gs = applyCandidate(gs, o);
			cout << gs << endl;
			if (gs.minWin()) {
				cout << "Player O wins!" << endl;
				break;
			}
			if (gs.isFilled()) {
				cout << "Tie: Cat's game!" << endl;
				break;
			}
		}
	}
	else {
		cpu = optionPrompt("Machine goes first (y/n)");
		if (cpu) {
			alpha_beta_on = optionPrompt("Alpha-beta pruning on (y/n) ");
			if (alpha_beta_on = true) { //machine goes first with ABP on
				while (true) {
					counter = 0;
					minimax(gs, act, val);
					gs = applyCandidate(gs, act);
					cout << gs << endl;
					if (gs.maxWin()) {
						cout << "Machine wins!" << endl;
						break;
					}
					if (gs.isFilled()) {
						cout << "Tie: Cat's game!" << endl;
						break;
					}

					action o;
					o.a = 1 << getValidChoice(gs, "Square? ");
					o.side = action::MIN;
					gs = applyCandidate(gs, o);

					cout << gs << endl;

					if (gs.minWin()) {
						cout << "Player wins!" << endl;
						break;
					}

					if (gs.isFilled()) {
						cout << "Tie: Cat's game!" << endl;
						break;
					}
				}
			}
			else {
				while (true) { //machine goes first with ABP off
					action x;
					srand(time(NULL));
					x.a = rand() % 8 << getValidChoice(gs, "");
					x.side = action::MAX;
					gs = applyCandidate(gs, x);
					cout << gs << endl;
					if (gs.maxWin()) {
						cout << "Machine wins!" << endl;
						break;
					}
					if (gs.isFilled()) {
						cout << "Tie: Cat's game!" << endl;
						break;
					}
					action o;
					o.a = 1 << getValidChoice(gs, "Square? ");
					o.side = action::MIN;
					gs = applyCandidate(gs, o);
					cout << gs << endl;
					if (gs.minWin()) {
						cout << "Player O wins!" << endl;
						break;
					}
					if (gs.isFilled()) {
						cout << "Tie: Cat's game!" << endl;
						break;
					}

				}

			}
		}
		else {
			alpha_beta_on = optionPrompt("Alpha-beta pruning on (y/n) ");
			if (alpha_beta_on = true) { //player goes first with ABP on
				while (true) {
					action x;
					x.a = 1 << getValidChoice(gs, "Player X choice? ");
					x.side = action::MIN;
					gs = applyCandidate(gs, x);
					cout << gs << endl;

					if (gs.maxWin()) {
						cout << "Player X wins!" << endl;
						break;
					}
					if (gs.isFilled()) {
						cout << "Tie: Cat's game!" << endl;
						break;
					}
					action o;
					counter = 0;
					minimax(gs, o, val);
					gs = applyCandidate(gs, o);
					cout << gs << endl;
					if (gs.maxWin()) {
						cout << "Machine wins!" << endl;
						break;
					}
					if (gs.isFilled()) {
						cout << "Tie: Cat's game!" << endl;
						break;
					}
				}
			}
			else
				//player plays as O and cpu plays as X
				while (true) {
					action x; //player goes first with ABP off
					action o;
					x.a = 1 << getValidChoice(gs, "Square? ");
					x.side = action::MIN;
					gs = applyCandidate(gs, x);
					cout << gs << endl;
					if (gs.maxWin()) {
						cout << "Player X wins!" << endl;
						break;
					}
					if (gs.isFilled()) {
						cout << "Tie: Cat's game!" << endl;
						break;
					}

					srand(time(NULL));
					o.a = rand() % 8 << getValidChoice(gs, "");
					gs = applyCandidate(gs, o);
					cout << gs << endl;
					if (gs.minWin()) {
						cout << "Machine wins!" << endl;
						break;
					}
					// If cat's game we're done
					if (gs.isFilled()) {
						cout << "Tie: Cat's game!" << endl;
						break;
					}
				}
		}
	}
}
/* pwflipper: Generate random passwords with your own coin flips.
 * Author: Ashley Holman <dscvlt@gmail.com>
 * http://github.com/ashleyholman/pwflipper
*/

#include <iostream>
#include <vector>
#include <termios.h>
#include <math.h>

#define DEFAULTSIZE 12

/* these are our alphabets for password generation */
const char keyboard[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`~!@#$%^&*()_-+={}[];:'\",<.>/?\\|";
const char base64[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/";
const char alphanum[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char alphanumlower[] = "abcdefghijklmnopqrstuvwxyz0123456789";
const char alphalower[] = "abcdefghijklmnopqrstuvwxyz";
const char digits[] = "0123456789";

// number of bits required to generate a character of cardinality strlen(alphabet) (determined at runtime)
int bitsreq = -1;
// original state of terminal, to restore when we're done
struct termios oldt;
// pointer to our alphabet .. default to 'keyboard'.
const char *alphabet = keyboard;
// cardinality of our character set
unsigned int cardinality;
// chance of having to restart
double restartChance;

void init() {
  // work out the number of characters in the alphabet being used
  cardinality = strlen(alphabet);

  // work out how many bits we need to generate
  unsigned int tmp = cardinality;
  while (tmp != 0) {
    tmp >>= 1;
    bitsreq++;
  }
  if (1 << bitsreq < cardinality) {
    // not an exact power of 2, so take the ceiling
    bitsreq++;
  }

  // set the restart chance
  restartChance = 1.0 - ((float)cardinality / (1 << bitsreq));

  // set the terminal to be character-buffered and turn off local echo
  struct termios newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON);
  newt.c_lflag &= ~(ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void die_usage() {
  std::cerr << "Usage: flippw [options] <N>" << std::endl;
  std::cerr << " <N>            Length of password to generate.  If unspecified, will default to 12." << std::endl;
  std::cerr << "--help          Display this help information." << std::endl;
  std::cerr << "--charset=NAME  Use the specified character set." << std::endl;
  std::cerr << "    Available character sets:" << std::endl;
  std::cerr << "    keyboard        All printable characters on a US keyboard. [strong]" << std::endl;
  std::cerr << "    base64          Uppercase and lowercase letters, digits, '+' and '/'." << std::endl;
  std::cerr << "    alphanum        Uppercase and lowercase letters + digits." << std::endl;
  std::cerr << "    alphanumlower   Lowercase letters + digits." << std::endl;
  std::cerr << "    alphalower      Lowercase letters only." << std::endl;
  std::cerr << "    digits          Decimal digits only. [weak]" << std::endl;
  exit(1);
}

bool process_args(int argc, char **argv, int &param1) {
  std::vector<std::string> args(argv+1, argv+argc);
  int params[] = {DEFAULTSIZE}, paramc = 0;
  for (std::vector<std::string>::iterator it = args.begin(); it != args.end(); it++) {
    std::string a = *it;
    if (a.substr(0, 2) == "--") {
      // this argument is a switch
      if (a.substr(2) == "help") {
        return false;
      } else if (a.substr(2,7) == "charset") {
        std::string charset = a.substr(10);
        if (charset == "keyboard")
          alphabet = keyboard;
        else if (charset == "base64")
          alphabet = base64;
        else if (charset == "alphanum")
          alphabet = alphanum;
        else if (charset == "alphanumlower")
          alphabet = alphanumlower;
        else if (charset == "alphalower")
          alphabet = alphalower;
        else if (charset == "digits")
          alphabet = digits;
        else {
          std::cerr << "Not a valid charset: " << charset << std::endl;
          return false;
        }
      } else {
        std::cerr << "Unknown option: " << a << std::endl << std::endl;
        return false;
      }
    } else {
      // parameter
      if (paramc == 0) {
        params[paramc++] = atoi(a.c_str());
      } else {
        // only expecting 1 param.
        std::cerr << "Too many arguments." << std::endl << std::endl;
        return false;
      }
    }
  }

  param1 = params[0];

  return true;
}

char flip_for_char(int chrnum) {
  static int flips = 0;
  // num gets constructed by the bits we flip
  unsigned int num = 0;

  for (int i = 0; i < bitsreq; i++) {
    ++flips;
    char result;
    std::cout << "Char " << chrnum << ", Flip " << flips << " [h|t]?>> ";
    while ((result = std::cin.get())) {
      if (result != 'h' && result != 't') {
        std::cout << " *** INVALID INPUT.  Specify 'h' or 't' ***" << std::endl;
        std::cout << "Char " << chrnum << ", Flip " << flips << " [h|t]?>> ";
      } else {
        // we've got our answer
        if (result == 'h') {
          // heads is a 0
          num <<= 1;
          std::cout << "HEADS!";
        } else {
          // tails is a 1
          num <<= 1;
          num |= 1;
          std::cout << "TAILS!";
          // make sure it's not impossible that this number can end up in the desired range.
          // generate theoretical result if all remaining flips are heads.
          int test = num << bitsreq-i-1;
          if (test >= cardinality) {
            // Oh no.  We can only generate a number >= cardinality (this happens when
            // cardinality is not a perfect power of 2 - we have to flip 2^k flips where
            // 2^k > cardinality > 2^(k-1).  In order to ensure a perfectly uniform
            // distribution of passwords, we need to totally disregard all work on
            // this character and start again.
            std::cout << std::endl << "Argh, sorry!  We need to start this character again...";
            num = 0;
            i=-1;
          }
        }
        std::cout << std::endl;
        break;
      }
    }
  }

  std::cout << "Character " << chrnum << " found: " << alphabet[num] << std::endl;
  return alphabet[num];
}

std::string flip_for_password(int length) {
  std::string pw = "";

  for (int i = 0; i < length; i++) {
    std::cout << "Flip for char. " << i+1 << std::endl;
    pw += flip_for_char(i+1);
  }

  return pw;
}

int main (int argc, char **argv) {
  int length;

  if (!process_args(argc, argv, length)) {
    die_usage();
  }

  // initialise fixed parameters for calculations
  init();

  std::cout << "LET'S GO FLIPPIN'! (length = " << length << ")" << std::endl;
  std::cout << "Chance of having to restart a character attempt with this charset: " << restartChance*100 << "%" << std::endl;
  std::string password = flip_for_password(length);

  std::cout << "CONGRATULATIONS!  Your flipped password is: " << password << std::endl;
  std::cout << "The keyspace size for this password is: " << powl(cardinality, length) << std::endl;
  std::cout << "Memorize it and stay safe!" << std::endl;

  // restore terminal state
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  return 0;
}

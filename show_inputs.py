from sys import argv

# USAGE: python3 show_inputs.py dump_file.txt output_file.txt

target_str = "Enter Command: "

with open(argv[1], "r") as f_in:
    with open(argv[2], "r") as f_out:
        print(f_out.readline().rstrip())
        print(f_out.readline().rstrip().replace("): ", f"): {f_in.readline().rstrip()}\n").replace(target_str, f"Enter Command: {f_in.readline().rstrip()}\n"))
        
        for line in f_out:
            line = line.rstrip()
            if target_str in line:
                line2 = f_in.readline().rstrip()
                line = line.replace(target_str, f"Enter Command: {line2}\n")
            print(line)

import os

# Get the directory of the script
script_dir = os.path.dirname(os.path.abspath(__file__))

# Specify the input hex file name and the output text file name
hex_file_name = "file.hex"
text_file_name = "file.txt"

# Create full paths to the input and output files
hex_file_path = os.path.join(script_dir, hex_file_name)
text_file_path = os.path.join(script_dir, text_file_name)

try:
    with open(hex_file_path, 'r') as hex_file:
        hex_data = hex_file.read()
except Exception as e:
    print(f"An error occurred while reading the hex file: {str(e)}")
    exit(1)

try:
    with open(text_file_path, 'w') as text_file:
        text_file.write(hex_data)
except Exception as e:
    print(f"An error occurred while writing the text file: {str(e)}")
    exit(1)

print(f"Conversion complete. The hex data has been saved to '{text_file_path}'.")

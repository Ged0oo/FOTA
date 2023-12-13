import os

# Get the current directory of the Python script
current_directory = os.path.dirname(os.path.abspath(__file__))

# Input binary file name
input_binary_file = 'Application.bin'
# Output text file name
output_text_file = 'Application.txt'

# Construct the full paths for the input and output files
input_binary_file_path = os.path.join(current_directory, input_binary_file)
output_text_file_path = os.path.join(current_directory, output_text_file)

with open(input_binary_file_path, 'rb') as binary_file:
    binary_data = binary_file.read()

# Convert binary data to a hexadecimal string
hex_string = ' '.join(f'{byte:02X}' for byte in binary_data)

# Write the hexadecimal string to the output text file
with open(output_text_file_path, 'w') as text_file:
    text_file.write(hex_string)

print(f'Binary file "{input_binary_file_path}" has been converted to text and saved as "{output_text_file_path}".')

import os

def count_lines_in_file(file_path):
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()
            return len(lines)
    except FileNotFoundError:
        return "File not found."

def count_lines_in_all_text_files():
    # Get the current directory
    current_directory = os.getcwd()
    
    # List all files in the current directory
    for file_name in os.listdir(current_directory):
        # Check if the file has a .txt extension
        if file_name.endswith('.txt'):
            file_path = os.path.join(current_directory, file_name)
            line_count = count_lines_in_file(file_path)
            print(f"File: {file_name} | Line Count: {line_count}")

count_lines_in_all_text_files()


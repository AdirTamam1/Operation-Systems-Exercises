#!/bin/bash

# NAME: Adir Tamam
# ID: 318936507

# Step 1: Verify that exactly 2 arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <source_pgn_file> <destination_directory>"
    exit 1
fi

# Step 2: Assign input arguments to variables
source_pgn_file="$1"
destination_dir="$2"

# Step 3: Check if the specified PGN file exists
if [ ! -f "$source_pgn_file" ]; then
    echo "Error: File '$source_pgn_file' does not exist."
    exit 1
fi

# Step 4: Ensure the destination directory exists
if [ ! -d "$destination_dir" ]; then
    mkdir -p "$destination_dir"  # Create the directory if it doesn't exist
    echo "Created directory '$destination_dir'."
fi

# Initialize variables for game tracking and content accumulation
game_number=1
game_content=""
in_game=false

# Function to save a game to a file
save_game() {
  local game_number="$1"
  local game_content="$2"
  local output_file="$destination_dir/$(basename "$source_pgn_file" .pgn)_$game_number.pgn"
  echo -e "$game_content" > "$output_file"
  echo "Saved game to '$output_file'"
}

# Step 5: Process the PGN file line by line
while IFS= read -r line || [[ -n "$line" ]]; do
    
    # Check if the line marks the start of a new game (starts with [Event)
    if [[ $line == "[Event "* ]]; then
        # If we are already in a game, save the current game data
        if $in_game; then
            save_game "$game_number" "$game_content"
            game_number=$((game_number + 1))  # Increment the game number
            game_content=""  # Clear the content for the next game
        fi
        # Mark that we are now processing a new game
        in_game=true
    fi
    
    # Append the current line (whether metadata or moves) to the game content
    game_content+="$line\n"
done < "$source_pgn_file"

# Step 6: Save the last game if there is any remaining content
if [[ -n "$game_content" ]]; then
    save_game "$game_number" "$game_content"
fi

# Final output message
echo "All games have been split and saved to '$destination_dir'."

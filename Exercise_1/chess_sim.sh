#!/bin/bash

# NAME: Adir Tamam
# ID: 318936507

# Check if the file exists
pgn_file="$1"
if [ ! -f "$pgn_file" ]; then
    echo "File does not exist: $pgn_file"
    exit 1
fi

# Get UCI moves from Python script (convert PGN to UCI format)
uci_moves=$(python3 parse_moves.py "$(cat "$pgn_file")")
uci_moves_array=($uci_moves)  # Convert Python output to an array

# Initialize variables
move_index=0
total_moves=${#uci_moves_array[@]}  # Total number of moves in the game

# Initialize the chessboard
initialize_board() {
    board=(
        r n b q k b n r
        p p p p p p p p
        . . . . . . . .
        . . . . . . . .
        . . . . . . . .
        . . . . . . . .
        P P P P P P P P
        R N B Q K B N R
    )
}

display_board() {
    local board=("${!1}")
    echo "  a b c d e f g h"
    for i in {8..1}; do
        echo -n "$i "
        for j in {0..7}; do
            echo -n "${board[$(( (8-i)*8 + j ))]} "
        done
        echo "$i"
    done
    echo "  a b c d e f g h"
}

apply_move() {

    local move=$1
    from_square="${move:0:2}"  # e.g., e2
    to_square="${move:2:2}"  # e.g., e4
    
    from_row=$((8 - ${from_square:1:1}))  # Rows are 1-8 in chess, but arrays are 0-7, so we subtract
    from_col=$(($(echo ${from_square:0:1} | tr 'a-h' '0-7')))  # Convert columns 'a' to 'h' to numbers 0-7
    
    to_row=$((8 - ${to_square:1:1}))  
    to_col=$(($(echo ${to_square:0:1} | tr 'a-h' '0-7')))

    # Store the piece to be moved in a temporary variable
    piece="${board[$((from_row * 8 + from_col))]}"

    from_index=$((8 * from_row + from_col))
    to_index=$((8 * to_row + to_col))

     # Handle castling
    # extra / bonus
    if [[ ("$move" == "e1g1" && "$piece" == "K") || ("$move" == "e8g8" && "$piece" == "k") ]]; then
        # Kingside castling
        if [[ "$move" == "e1g1" ]]; then
            board[60]="."
            board[61]="R"
            board[62]="K"
            board[63]="."
        else
            board[4]="."
            board[5]="r"
            board[6]="k"
            board[7]="."
        fi
        return
    elif [[ ("$move" == "e1c1" && "$piece" == "K") || ("$move" == "e8c8" && "$piece" == "k") ]]; then
        # Queenside castling
        if [[ "$move" == "e1c1" ]]; then
            board[56]="."
            board[58]="K"
            board[59]="R"
            board[60]="."
        else
            board[0]="."
            board[2]="k"
            board[3]="r"
            board[4]="."
        fi
        return
    fi

    # Handle en passant
    # extra / bonus
    if [[ "${board[$from_index]}" == "P" && "${board[$to_index]}" == "." && "$from_column" != "$to_column" && "${from_row}" == "5" ]]; then
        board[$to_index]="P"
        board[$((to_index + 8))]="."
        board[$from_index]="."
        return
    elif [[ "${board[$from_index]}" == "p" && "${board[$to_index]}" == "." && "$from_column" != "$to_column" && "${from_row}" == "4" ]]; then
        board[$to_index]="p"
        board[$((to_index - 8))]="."
        board[$from_index]="."
        return
    fi

    # Handle pawn promotion
    # check if the move have 5 characters, if so, it is a promotion
    # the last character is the piece that the pawn is going to be promoted
    if [ ${#move} -eq 5 ]; then
        local promotion_piece=${move:4:1}
        # check if the promotion piece is a valid piece
        # =~ is a regolar expresion, it is used to check if the promotion piece is a valid piece
        if [[ "$promotion_piece" =~ [QRBNqrnb] ]]; then
            board[$to_index]=$promotion_piece
            board[$from_index]="."
            return
        fi
    fi

    # Handle normal move
    board[$to_index]=${board[$from_index]}
    board[$from_index]="."
}




# Function to display metadata from the PGN file
display_metadata() {
    echo "Metadata from PGN file:"
    # Extract and print metadata from PGN file
    grep -E "^\[.*\]" "$pgn_file"
    echo "Move $move_index/$total_moves"
}

# Initialize the board
initialize_board

# Display the metadata first
display_metadata
# Display Board
display_board board[@]

# Main interaction loop
while true; do
    
    # Print the prompt on the same line
    echo -n "Press 'd' to move forward, 'a' to move back, 'w' to go to the start, 's' to go to the end, 'q' to quit: "
    
    # Read a single key press without a newline
    read key  # Read a single key press
    case $key in
        d) 
            if [ $move_index -lt $total_moves ]; then
                uci_move="${uci_moves_array[$move_index]}"  # Get the UCI move
                move_index=$((move_index + 1))
                
                apply_move "$uci_move"   # Update the board with the move
                # Clear the line and print the updated move and board
                echo "Move $move_index/$total_moves"  # Clear the line before printing
                display_board board[@]  # Display the updated board
            else
                # Clear the line and show that no more moves are available
                echo "No more moves available."
            fi
            ;;
        a)  
            if [ $move_index -gt 0 ]; then
                move_index=$((move_index - 1))
                i=0 
                initialize_board
                while [ $i -lt $move_index ]; do
                uci_move="${uci_moves_array[$i]}"  # Get the UCI move
                i=$((i + 1))  # Increment the move index
                apply_move "$uci_move"  # Apply the move to the board
                done
            fi
                # Revert to previous move (this would need a history array for complete undo functionality)
                echo  "Move $move_index/$total_moves"
                display_board board[@]
            
            ;;
        w)  
            move_index=0
            initialize_board
            echo Move $move_index/$total_moves
            display_board board[@]
            ;;
       s)  
            while [ $move_index -lt $total_moves ]; do
                uci_move="${uci_moves_array[$move_index]}"  # Get the UCI move
                move_index=$((move_index + 1))  # Increment the move index
                apply_move "$uci_move"  # Apply the move to the board
            done
            echo "Move $move_index/$total_moves"  # Print the current move index and total moves
            display_board board[@]
            ;;

        q) 
            echo "Exiting."
            echo "End of game."
            break
            ;;
        *)  
            echo "Invalid key pressed: $key"
            ;;
    esac
done
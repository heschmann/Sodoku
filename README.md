# Sudoku
 Simple Sudoku solver using recursive backtracking in C++.

Two strategies are implemented.
In the first the cell is selected with the fewest options. This resulted in a significant speedup.
Another strategy is to reduce the options as far as possible before starting the RBF procedure.
More precisely, for every cell it is checked wether it is the only cell with a given option in its row, col, or block.
While this approach worked well the additional checking that needs to be done does usually not pay off as the RBF is very fast and simple and usually does not search very deep before encountering infeasible solutions.
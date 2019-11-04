#include "includes.h"
#include "chess.h"


// -------------------------------------------------------------------
// Chess class
// -------------------------------------------------------------------
int Chess::getPieceColor(char chPiece)
{
   if (isupper(chPiece))
   {
      return WHITE_PIECE;
   }
   else
   {
      return BLACK_PIECE;
   }
}

bool Chess::isWhitePiece(char chPiece)
{
   return getPieceColor(chPiece) == Chess::WHITE_PIECE ? true : false;
}

bool Chess::isBlackPiece(char chPiece)
{
   return getPieceColor(chPiece) == Chess::BLACK_PIECE ? true : false;
}

std::string Chess::describePiece(char chPiece)
   {
      std::string description;

      if (isWhitePiece(chPiece))
      {
         description += "White ";
      }
      else
      {
         description += "Black ";
      }

      switch (toupper(chPiece))
      {
         case 'P':
         {
            description += "pawn";
         }
         break;

         case 'N':
         {
            description += "knight";
         }
         break;

         case 'B':
         {
            description += "bishop";
         }
         break;

         case 'R':
         {
            description += "rook";
         }
         break;

         case 'Q':
         {
            description += "queen";
         }
         break;

         default:
         {
            description += "unknow piece";
         }
         break;
      }

      return description;
   }


// -------------------------------------------------------------------
// Game class
// -------------------------------------------------------------------
Game::Game(IErrorReporter& reporter, const ChessBoard board, int round,
           Chess::Position LastMoveFrom, Chess::Position LastMoveTo,
           bool bCastlingKingSideWhiteAllowed,
           bool bCastlingKingSideBlackAllowed,
           bool bCastlingQueenSideWhiteAllowed,
           bool bCastlingQueenSideBlackAllowed)
           :m_reporter(reporter),
           m_LastMoveFrom(LastMoveFrom),
           m_LastMoveTo(LastMoveTo)
{
   // Game on!
   m_bGameFinished = false;

   // Initial board settings
   memcpy(this->board.piece, board.piece, sizeof(char) * 8 * 8);
   m_round = round;

   // Castling is allowed (to each side) until the player moves the king or the rook
   m_bCastlingKingSideAllowed[WHITE_PLAYER]  = bCastlingKingSideWhiteAllowed;
   m_bCastlingKingSideAllowed[BLACK_PLAYER]  = bCastlingKingSideBlackAllowed;

   m_bCastlingQueenSideAllowed[WHITE_PLAYER] = bCastlingQueenSideWhiteAllowed;
   m_bCastlingQueenSideAllowed[BLACK_PLAYER] = bCastlingQueenSideBlackAllowed;
}

Game::~Game()
{

}

//---------------------------------------------------------------------------------------
// Helper
// Auxiliar functions to determine if a move is valid, etc
//---------------------------------------------------------------------------------------
bool Game::isMoveValid(Chess::Position present, Chess::Position future,
                       Chess::EnPassant* S_enPassant, Chess::Castling* S_castling,
                       Chess::Promotion* S_promotion,
                       Chess::PawnDoubleMove* pawndoublemove)
{
   bool bValid = false;
   pawndoublemove->bWasDoubleMove = false;


   char chPiece = getPieceAtPosition(present.iRow, present.iColumn);

   // ----------------------------------------------------
   // 1. Is the piece  allowed to move in that direction?
   // ----------------------------------------------------
   switch( toupper(chPiece) )
   {
      case 'P':
      {
         // Wants to move forward
         if ( future.iColumn == present.iColumn )
         {
            // Simple move forward
            if ( (Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 1) ||
                 (Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 1) )
            {
               if ( EMPTY_SQUARE == getPieceAtPosition(future.iRow, future.iColumn) )
               {
                  bValid = true;
               }
            }

            // Double move forward
            else if ( (Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 2) ||
                      (Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 2) )
            {
               // This is only allowed if the pawn is in its original place
               if ( Chess::isWhitePiece(chPiece) )
               {
                  if ( EMPTY_SQUARE == getPieceAtPosition(future.iRow-1, future.iColumn) &&
                       EMPTY_SQUARE == getPieceAtPosition(future.iRow, future.iColumn)   &&
                                1   == present.iRow )
                  {
                     bValid = true;
                     pawndoublemove->bWasDoubleMove = true;
                  }
               }
               else // if ( isBlackPiece(chPiece) )
               {
                  if ( EMPTY_SQUARE == getPieceAtPosition(future.iRow + 1, future.iColumn) &&
                       EMPTY_SQUARE == getPieceAtPosition(future.iRow, future.iColumn)     &&
                                6   == present.iRow)
                  {
                     bValid = true;
                     pawndoublemove->bWasDoubleMove = true;
                  }
               }
            }
            else
            {
               // This is invalid
               return false;
            }
         }

         // The "en passant" move
         else if ( ( (Chess::isWhitePiece(chPiece) && 4 == present.iRow && 5 == future.iRow && 1 == abs(future.iColumn - present.iColumn) ) ||
                   (Chess::isBlackPiece(chPiece) && 3 == present.iRow && 2 == future.iRow && 1 == abs(future.iColumn - present.iColumn) ) ) &&
                   // Did the pawn have a double move forward and was it an adjacent column?
                   ( 2 == abs(m_LastMoveTo.iRow - m_LastMoveFrom.iRow) && 1 == abs(m_LastMoveFrom.iColumn - present.iColumn) )
                 )
         {
            // First of all, was it a pawn?
            char chLstMvPiece = getPieceAtPosition(m_LastMoveTo.iRow, m_LastMoveTo.iColumn);

            if (toupper(chLstMvPiece) != 'P')
            {
               return false;
            }

             // En passant move!
             bValid = true;

             S_enPassant->bApplied = true;
             S_enPassant->PawnCaptured.iRow    = m_LastMoveTo.iRow;
             S_enPassant->PawnCaptured.iColumn = m_LastMoveTo.iColumn;
         }

         // Wants to capture a piece
         else if (1 == abs(future.iColumn - present.iColumn))
         {
            if ( (Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 1) || (Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 1))
            {
               // Only allowed if there is something to be captured in the square
               if (EMPTY_SQUARE != getPieceAtPosition(future.iRow, future.iColumn))
               {
                  bValid = true;
                  // Pawn captured a piece!
               }
            }
         }
         else
         {
            // This is invalid
            return false;
         }

         // If a pawn reaches its eight rank, it must be promoted to another piece
         if ( (Chess::isWhitePiece( chPiece ) && 7 == future.iRow) ||
              (Chess::isBlackPiece( chPiece ) && 0 == future.iRow) )
         {
            // Pawn must be promoted!\n
            S_promotion->bApplied = true;
         }
      }
      break;

      case 'R':
      {
         // Horizontal move
         if ( (future.iRow == present.iRow) && (future.iColumn != present.iColumn) )
         {
            // Check if there are no pieces on the way
            if ( isPathFree(present, future, Chess::HORIZONTAL) )
            {
               bValid = true;
            }
         }
         // Vertical move
         else if ( (future.iRow != present.iRow) && (future.iColumn == present.iColumn) )
         {
            // Check if there are no pieces on the way
            if ( isPathFree(present, future, Chess::VERTICAL) )
            {
               bValid = true;
            }
         }
      }
      break;

      case 'N':
      {
         if ( (2 == abs(future.iRow - present.iRow)) && (1 == abs(future.iColumn - present.iColumn)) )
         {
            bValid = true;
         }

         else if (( 1 == abs(future.iRow - present.iRow)) && (2 == abs(future.iColumn - present.iColumn)) )
         {
            bValid = true;
         }
      }
      break;

      case 'B':
      {
         // Diagonal move
         if ( abs(future.iRow - present.iRow) == abs(future.iColumn - present.iColumn) )
         {
            // Check if there are no pieces on the way
            if ( isPathFree(present, future, Chess::DIAGONAL) )
            {
               bValid = true;
            }
         }
      }
      break;

      case 'Q':
      {
         // Horizontal move
         if ( (future.iRow == present.iRow) && (future.iColumn != present.iColumn) )
         {
            // Check if there are no pieces on the way
            if ( isPathFree(present, future, Chess::HORIZONTAL))
            {
               bValid = true;
            }
         }
         // Vertical move
         else if ( (future.iRow != present.iRow) && (future.iColumn == present.iColumn) )
         {
            // Check if there are no pieces on the way
            if ( isPathFree(present, future, Chess::VERTICAL))
            {
               bValid = true;
            }
         }

         // Diagonal move
         else if ( abs(future.iRow - present.iRow) == abs(future.iColumn - present.iColumn) )
         {
            // Check if there are no pieces on the way
            if ( isPathFree(present, future, Chess::DIAGONAL))
            {
               bValid = true;
            }
         }
      }
      break;

      case 'K':
      {
         // Horizontal move by 1
         if ( (future.iRow == present.iRow) && (1 == abs(future.iColumn - present.iColumn) ) )
         {
            bValid = true;
         }

         // Vertical move by 1
         else if ( (future.iColumn == present.iColumn) && (1 == abs(future.iRow - present.iRow) ) )
         {
            bValid = true;
         }

         // Diagonal move by 1
         else if ( (1 == abs(future.iRow - present.iRow) ) && (1 == abs(future.iColumn - present.iColumn) ) )
         {
            bValid = true;
         }

         // Castling
         else if ( (future.iRow == present.iRow) && (2 == abs(future.iColumn - present.iColumn) ) )
         {
            // Castling is only allowed in these circunstances:

            // 1. King is not in check
            if ( true == playerKingInCheck() )
            {
               return false;
            }

            // 2. No pieces in between the king and the rook
            if ( false == isPathFree( present, future, Chess::HORIZONTAL ) )
            {
               return false;
            }

            // 3. King and rook must not have moved yet;
            // 4. King must not pass through a square that is attacked by an enemy piece
            if ( future.iColumn > present.iColumn )
            {
               // if future.iColumn is greather, it means king side
               if ( false == castlingAllowed(Chess::Side::KING_SIDE, Chess::getPieceColor(chPiece) ) )
               {
                  m_reporter.ReportError("Castling to the king side is not allowed.\n");
                  return false;
               }
               else
               {
                  // Check if the square that the king skips is not under attack
                  Chess::UnderAttack square_skipped = isUnderAttack( present.iRow, present.iColumn + 1, getCurrentTurn() );
                  if ( false == square_skipped.bUnderAttack )
                  {
                     // Fill the S_castling structure
                     S_castling->bApplied = true;

                     // Present position of the rook
                     S_castling->rook_before.iRow    = present.iRow;
                     S_castling->rook_before.iColumn = present.iColumn + 3;

                     // Future position of the rook
                     S_castling->rook_after.iRow    = future.iRow;
                     S_castling->rook_after.iColumn = present.iColumn + 1; // future.iColumn -1

                     bValid = true;
                  }
               }
            }
            else //if (future.iColumn < present.iColumn)
            {
               // if present.iColumn is greather, it means queen side
               if (false == castlingAllowed(Chess::Side::QUEEN_SIDE, Chess::getPieceColor(chPiece)))
               {
                  m_reporter.ReportError("Castling to the queen side is not allowed.\n");
                  return false;
               }
               else
               {
                  // Check if the square that the king skips is not attacked
                  Chess::UnderAttack square_skipped = isUnderAttack( present.iRow, present.iColumn - 1, getCurrentTurn() );
                  if ( false == square_skipped.bUnderAttack )
                  {
                     // Fill the S_castling structure
                     S_castling->bApplied = true;

                     // Present position of the rook
                     S_castling->rook_before.iRow    = present.iRow;
                     S_castling->rook_before.iColumn = present.iColumn - 4;

                     // Future position of the rook
                     S_castling->rook_after.iRow    = future.iRow;
                     S_castling->rook_after.iColumn = present.iColumn - 1; // future.iColumn +1

                     bValid = true;
                  }
               }
            }
         }
      }
      break;

      default:
      {
        char chBuff[255];
        snprintf(chBuff, 256, "!!!!Should not reach here. Invalid  in in isMoveValid piece=%i row=%i col=%i", chPiece, present.iRow, present.iColumn);
         m_reporter.ReportError(chBuff);
      }
      break;
   }

   // If it is a move in an invalid direction, do not even bother to check the rest
   if ( false == bValid )
   {
      // Piece is not allowed to move to that square
      return false;
   }


   // -------------------------------------------------------------------------
   // 2. Is there another piece of the same color on the destination square?
   // -------------------------------------------------------------------------
   if (isSquareOccupied(future.iRow, future.iColumn))
   {
      char chAuxPiece = getPieceAtPosition(future.iRow, future.iColumn);
      if ( Chess::getPieceColor(chPiece) == Chess::getPieceColor(chAuxPiece) )
      {
         // Position is already taken by a piece of the same color
         return false;
      }
   }

   // ----------------------------------------------
   // 3. Would the king be in check after the move?
   // ----------------------------------------------
   if ( true == wouldKingBeInCheck(chPiece, present, future, S_enPassant) )
   {
      // Move would put player's king in check
      return false;
   }

   return bValid;
}

void Game::movePiece(Position present, Position future, Chess::EnPassant* S_enPassant,
                     Chess::Castling* S_castling, Chess::Promotion* S_promotion,
                     Chess::Capture* S_capture)
{
   // Get the piece to be moved
   char chPiece = getPieceAtPosition(present);

   // Is the destination square occupied?
   char chCapturedPiece = getPieceAtPosition(future);

   // So, was a piece captured in this move?
   if (0x20 != chCapturedPiece)
   {
     S_capture->bCaptured = true;
   }
   else if (true == S_enPassant->bApplied)
   {
      char chCapturedEP = getPieceAtPosition(S_enPassant->PawnCaptured.iRow, S_enPassant->PawnCaptured.iColumn);

      // Now, remove the captured pawn
      board.piece[S_enPassant->PawnCaptured.iRow][S_enPassant->PawnCaptured.iColumn] = EMPTY_SQUARE;

      S_capture->bCaptured = true;
   }
   else
   {
     S_capture->bCaptured = false;
   }

   // Remove piece from present position
   board.piece[present.iRow][present.iColumn] = EMPTY_SQUARE;

   // Move piece to new position
   if ( true == S_promotion->bApplied )
   {
      board.piece[future.iRow][future.iColumn] = S_promotion->chAfter;
   }
   else
   {
      board.piece[future.iRow][future.iColumn] = chPiece;
   }

   // Was it a castling move?
   if ( S_castling->bApplied == true  )
   {
      // The king was already move, but we still have to move the rook to 'jump' the king
      char chPiece = getPieceAtPosition(S_castling->rook_before.iRow, S_castling->rook_before.iColumn);

      // Remove the rook from present position
      board.piece[S_castling->rook_before.iRow][S_castling->rook_before.iColumn] = EMPTY_SQUARE;

      // 'Jump' into to new position
      board.piece[S_castling->rook_after.iRow][S_castling->rook_after.iColumn] = chPiece;
   }
   else
   {
   }

   // Castling requirements
   if ( 'K' == toupper(chPiece) )
   {
      // After the king has moved once, no more castling allowed
      m_bCastlingKingSideAllowed[getCurrentTurn()]  = false;
      m_bCastlingQueenSideAllowed[getCurrentTurn()] = false;
   }
   else if ('R' == toupper(chPiece) )
   {
      // If the rook moved from column 'A', no more castling allowed on the queen side
      if ( 0 == present.iColumn )
      {
         m_bCastlingQueenSideAllowed[getCurrentTurn()] = false;
      }

      // If the rook moved from column 'A', no more castling allowed on the queen side
      else if ( 7 == present.iColumn )
      {
         m_bCastlingKingSideAllowed[getCurrentTurn()] = false;
      }
   }

   // Change turns
   changeTurns();
}

bool Game::castlingAllowed(Side iSide, int iColor)
{
   if ( QUEEN_SIDE == iSide )
   {
      return m_bCastlingQueenSideAllowed[iColor];
   }
   else //if ( KING_SIDE == iSide )
   {
      return m_bCastlingKingSideAllowed[iColor];
   }
}

char Game::getPieceAtPosition(int iRow, int iColumn)
{
   return board.piece[iRow][iColumn];
}

char Game::getPieceAtPosition(Position pos)
{
   return board.piece[pos.iRow][pos.iColumn];
}

char Game::getPiece_considerMove(int iRow, int iColumn, IntendedMove* intended_move)
{
   char chPiece;

   // If there is no intended move, just return the current position of the board
   if (nullptr == intended_move)
   {
      chPiece = getPieceAtPosition(iRow, iColumn);
   }
   else
   {
      // In this case, we are trying to understand what WOULD happed if the move was made,
      // so we consider a move that has not been made yet
      if (intended_move->from.iRow == iRow && intended_move->from.iColumn == iColumn)
      {
         // The piece wants to move from that square, so it would be empty
         chPiece = EMPTY_SQUARE;
      }
      else if (intended_move->to.iRow == iRow && intended_move->to.iColumn == iColumn)
      {
         // The piece wants to move to that square, so return the piece
         chPiece = intended_move->chPiece;
      }
      else
      {
         chPiece = getPieceAtPosition(iRow, iColumn);
      }
   }

   return chPiece;
}

Chess::UnderAttack Game::isUnderAttack(int iRow, int iColumn, int iColor, IntendedMove* pintended_move)
{
   UnderAttack attack = { 0 };

   // a) Direction: HORIZONTAL
   {
      // Check all the way to the right
      for (int i = iColumn + 1; i < 8; i++)
      {
         char chPieceFound = getPiece_considerMove(iRow, i, pintended_move);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'R') )
         {
            // There is a queen or a rook to the right, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow = iRow;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = i;
            attack.attacker[attack.iNumAttackers-1].dir = HORIZONTAL;
            break;
         }
         else if ( (i == iColumn + 1) && (toupper(chPieceFound) == 'K') )
         {
            // There is king one space to the right, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow = iRow;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = i;
            attack.attacker[attack.iNumAttackers-1].dir = HORIZONTAL;
            break;
         }
         else
         {
            // There is a piece that does not attack horizontally, so no problem
            break;
         }
      }

      // Check all the way to the left
      for (int i = iColumn - 1; i >= 0; i--)
      {
         char chPieceFound = getPiece_considerMove(iRow, i, pintended_move);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if  (iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'R') )
         {
            // There is a queen or a rook to the right, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = iRow;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = i;
            attack.attacker[attack.iNumAttackers-1].dir = HORIZONTAL;
            break;
         }
         else if ( (i == iColumn - 1) && (toupper(chPieceFound) == 'K') )
         {
            // There is a king one space to the right, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = iRow;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = i;
            attack.attacker[attack.iNumAttackers-1].dir = HORIZONTAL;
            break;
         }
         else
         {
            // There is a piece that does not attack horizontally, so no problem
            break;
         }
      }
   }

   // b) Direction: VERTICAL
   {
      // Check all the way up
      for (int i = iRow + 1; i < 8; i++)
      {
         char chPieceFound = getPiece_considerMove(i, iColumn, pintended_move);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'R')  )
         {
            // There is a queen or a rook above, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = iColumn;
            attack.attacker[attack.iNumAttackers-1].dir = VERTICAL;
            break;
         }
         else if ( (i == iRow + 1) && (toupper(chPieceFound) == 'K')  )
         {
            // There is a king one space up, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = iColumn;
            attack.attacker[attack.iNumAttackers-1].dir = VERTICAL;
            break;
         }
         else
         {
            // There is a piece that does not attack vertically, so no problem
            break;
         }
      }

      // Check all the way down
      for (int i = iRow - 1; i >= 0; i--)
      {
         char chPieceFound = getPiece_considerMove(i, iColumn, pintended_move);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ((toupper(chPieceFound) == 'Q') ||
                  (toupper(chPieceFound) == 'R') )
         {
            // There is a queen or a rook to the right, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = iColumn;
            attack.attacker[attack.iNumAttackers-1].dir = VERTICAL;
            break;
         }
         else if ((i == iRow - 1) && (toupper(chPieceFound) == 'K') )
         {
            // There is a king one space to the below, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = iColumn;
            attack.attacker[attack.iNumAttackers-1].dir = VERTICAL;
            break;
         }
         else
         {
            // There is a piece that does not attack vertically, so no problem
            break;
         }
      }
   }

   // c) Direction: DIAGONAL
   {
      // Check the diagonal up-right
      for (int i = iRow + 1, j = iColumn + 1; i < 8 && j < 8; i++, j++)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    + 1        ) &&
                   (   j   == iColumn + 1        ) &&
                   (iColor == WHITE_PIECE ) )
         {
            // A pawn only puts another piece in jeopardy if it's (diagonally) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B'))
         {
            // There is a queen or a bishop in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (i == iRow + 1) && (toupper(chPieceFound) == 'K'))
         {
            // There is a king in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else
         {
            // There is a piece that does not attack diagonally, so no problem
            break;
         }
      }

      // Check the diagonal up-left
      for (int i = iRow + 1, j = iColumn - 1; i < 8 && j >= 0; i++, j--)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    + 1        ) &&
                   (   j   == iColumn - 1        ) &&
                   (iColor == WHITE_PIECE ) )
         {
            // A pawn only puts another piece in jeopardy if it's (diagonally) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B'))
         {
            // There is a queen or a bishop in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (i == iRow + 1) && (toupper(chPieceFound) == 'K'))
         {
            // There is a king in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else
         {
            // There is a piece that does not attack diagonally, so no problem
            break;
         }
      }

      // Check the diagonal down-right
      for (int i = iRow - 1, j = iColumn + 1; i >= 0 && j < 8; i--, j++)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    - 1        ) &&
                   (   j   == iColumn + 1        ) &&
                   (iColor == BLACK_PIECE ) )
         {
            // A pawn only puts another piece in jeopardy if it's (diagonally) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B') )
         {
            // There is a queen or a bishop in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (i == iRow - 1) && (toupper(chPieceFound) == 'K') )
         {
            // There is a qking in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else
         {
            // There is a piece that does not attack diagonally, so no problem
            break;
         }
      }

      // Check the diagonal down-left
      for (int i = iRow - 1, j = iColumn - 1; i >= 0 && j >= 0; i--, j--)
      {
         char chPieceFound = getPiece_considerMove(i, j, pintended_move);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color, so no problem
            break;
         }
         else if ( (toupper(chPieceFound) == 'P' ) &&
                   (   i   == iRow    - 1        ) &&
                   (   j   == iColumn - 1        ) &&
                   (iColor == BLACK_PIECE ) )
         {
            // A pawn only puts another piece in jeopardy if it's (diagonally) right next to it
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                     (toupper(chPieceFound) == 'B') )
         {
            // There is a queen or a bishop in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else if ( (i == iRow - 1) && (toupper(chPieceFound) == 'K') )
         {
            // There is a king in that direction, so the piece is in jeopardy
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = i;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = j;
            attack.attacker[attack.iNumAttackers-1].dir = DIAGONAL;
            break;
         }
         else
         {
            // There is a piece that does not attack diagonally, so no problem
            break;
         }
      }
   }

   // d) Direction: L_SHAPED
   {
      // Check if the piece is put in jeopardy by a knigh

      Position knight_moves[8] = { {  1, -2 }, {  2, -1 }, {  2, 1 }, {  1, 2 },
                                   { -1, -2 }, { -2, -1 }, { -2, 1 }, { -1, 2 } };
      for (int i = 0; i < 8; i++)
      {
         int iRowToTest    = iRow    + knight_moves[i].iRow;
         int iColumnToTest = iColumn + knight_moves[i].iColumn;

         if (iRowToTest < 0 || iRowToTest > 7 || iColumnToTest < 0 || iColumnToTest > 7)
         {
            // This square does not even exist, so no need to test
            continue;
         }

         char chPieceFound = getPiece_considerMove(iRowToTest, iColumnToTest, pintended_move);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if (iColor == getPieceColor(chPieceFound))
         {
            // This is a piece of the same color, so no problem
            continue;
         }
         else if ( (toupper(chPieceFound) == 'N') )
         {
            attack.bUnderAttack = true;
            attack.iNumAttackers += 1;

            attack.attacker[attack.iNumAttackers-1].pos.iRow    = iRowToTest;
            attack.attacker[attack.iNumAttackers-1].pos.iColumn = iColumnToTest;
            break;
         }
      }
   }

   return attack;
}

bool Game::isReachable( int iRow, int iColumn, int iColor )
{
   bool bReachable = false;

   // a) Direction: HORIZONTAL
   {
      // Check all the way to the right
      for (int i = iColumn + 1; i < 8; i++)
      {
         char chPieceFound = getPieceAtPosition(iRow, i);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'R') )
         {
            // There is a queen or a rook to the right, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move horizontally
            break;
         }
      }

      // Check all the way to the left
      for (int i = iColumn - 1; i >= 0; i--)
      {
         char chPieceFound = getPieceAtPosition(iRow, i);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if  (iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'R') )
         {
            // There is a queen or a rook to the left, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move horizontally
            break;
         }
      }
   }

   // b) Direction: VERTICAL
   {
      // Check all the way up
      for (int i = iRow + 1; i < 8; i++)
      {
         char chPieceFound = getPieceAtPosition(i, iColumn);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound)       == 'P' )         &&
                   (getPieceColor(chPieceFound) == BLACK_PIECE ) &&
                   ( i  == iRow + 1 )                            )
         {
            // There is a pawn one square up, so the square is reachable
            bReachable = true;
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'R')  )
         {
            // There is a queen or a rook on the way up, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move vertically
            break;
         }
      }

      // Check all the way down
      for (int i = iRow - 1; i >= 0; i--)
      {
         char chPieceFound = getPieceAtPosition(i, iColumn);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound)       == 'P' )         &&
                   (getPieceColor(chPieceFound) == WHITE_PIECE ) &&
                   ( i  == iRow - 1 )                            )
         {
            // There is a pawn one square down, so the square is reachable
            bReachable = true;
            break;
         }
         else if ((toupper(chPieceFound) == 'Q') ||
                  (toupper(chPieceFound) == 'R') )
         {
            // There is a queen or a rook on the way down, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move vertically
            break;
         }
      }
   }

   // c) Direction: DIAGONAL
   {
      // Check the diagonal up-right
      for (int i = iRow + 1, j = iColumn + 1; i < 8 && j < 8; i++, j++)
      {
         char chPieceFound = getPieceAtPosition(i, j);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B'))
         {
            // There is a queen or a bishop in that direction, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move diagonally
            break;
         }
      }

      // Check the diagonal up-left
      for (int i = iRow + 1, j = iColumn - 1; i < 8 && j > 0; i++, j--)
      {
         char chPieceFound = getPieceAtPosition(i, j);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B'))
         {
            // There is a queen or a bishop in that direction, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move diagonally
            break;
         }
      }

      // Check the diagonal down-right
      for (int i = iRow - 1, j = iColumn + 1; i > 0 && j < 8; i--, j++)
      {
         char chPieceFound = getPieceAtPosition(i, j);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B') )
         {
            // There is a queen or a bishop in that direction, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move diagonally
            break;
         }
      }

      // Check the diagonal down-left
      for (int i = iRow - 1, j = iColumn - 1; i > 0 && j > 0; i--, j--)
      {
         char chPieceFound = getPieceAtPosition(i, j);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if ( iColor == getPieceColor(chPieceFound) )
         {
            // This is a piece of the same color
            break;
         }
         else if ( (toupper(chPieceFound) == 'Q') ||
                   (toupper(chPieceFound) == 'B') )
         {
            // There is a queen or a bishop in that direction, so the square is reachable
            bReachable = true;
            break;
         }
         else
         {
            // There is a piece that does not move diagonally
            break;
         }
      }
   }

   // d) Direction: L_SHAPED
   {
      // Check if the piece is put in jeopardy by a knigh

      Position knight_moves[8] = { {  1, -2 }, {  2, -1 }, {  2, 1 }, {  1, 2 },
                                   { -1, -2 }, { -2, -1 }, { -2, 1 }, { -1, 2 } };
      for (int i = 0; i < 8; i++)
      {
         int iRowToTest    = iRow    + knight_moves[i].iRow;
         int iColumnToTest = iColumn + knight_moves[i].iColumn;

         if (iRowToTest < 0 || iRowToTest > 7 || iColumnToTest < 0 || iColumnToTest > 7)
         {
            // This square does not even exist, so no need to test
            continue;
         }

         char chPieceFound = getPieceAtPosition(iRowToTest, iColumnToTest);
         if (EMPTY_SQUARE == chPieceFound)
         {
            // This square is empty, move on
            continue;
         }

         if (iColor == getPieceColor(chPieceFound))
         {
            // This is a piece of the same color
            continue;
         }
         else if ( (toupper(chPieceFound) == 'N') )
         {
            bReachable = true;
            break;
         }
      }
   }

   return bReachable;
}

bool Game::isSquareOccupied(int iRow, int iColumn)
{
   bool bOccupied = false;

   if ( 0x20 != getPieceAtPosition(iRow,iColumn) )
   {
      bOccupied = true;
   }

   return bOccupied;
}

bool Game::isPathFree(Position startingPos, Position finishingPos, int iDirection)
{
   bool bFree = false;

   switch(iDirection)
   {
      case Chess::HORIZONTAL:
      {
         // If it is a horizontal move, we can assume the startingPos.iRow == finishingPos.iRow
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iColumn == finishingPos.iColumn)
         {
            //printf("Error. Movement is horizontal but column is the same\n");
         }

         // Moving to the right
         else if (startingPos.iColumn < finishingPos.iColumn)
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = startingPos.iColumn + 1; i < finishingPos.iColumn; i++)
            {
               if (isSquareOccupied(startingPos.iRow, i))
               {
                  bFree = false;
                  //printf("Horizontal path to the right is not clear!\n");
               }
            }
         }

         // Moving to the left
         else //if (startingPos.iColumn > finishingPos.iColumn)
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = startingPos.iColumn - 1; i > finishingPos.iColumn; i--)
            {
               if (isSquareOccupied(startingPos.iRow, i))
               {
                  bFree = false;
                  //printf("Horizontal path to the left is not clear!\n");
               }
            }
         }
      }
      break;

      case Chess::VERTICAL:
      {
         // If it is a vertical move, we can assume the startingPos.iColumn == finishingPos.iColumn
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iRow == finishingPos.iRow)
         {
            m_reporter.ReportError("Error. Movement is vertical but row is the same\n");
         }

         // Moving up
         else if (startingPos.iRow < finishingPos.iRow)
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = startingPos.iRow + 1; i < finishingPos.iRow; i++)
            {
               if ( isSquareOccupied(i, startingPos.iColumn) )
               {
                  bFree = false;
                  //printf("Vertical path up is not clear!\n");
               }
            }
         }

         // Moving down
         else //if (startingPos.iColumn > finishingPos.iRow)
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = startingPos.iRow - 1; i > finishingPos.iRow; i--)
            {
               if ( isSquareOccupied(i, startingPos.iColumn) )
               {
                  bFree = false;
                  //printf("Vertical path down is not clear!\n");
               }
            }
         }
      }
      break;

      case Chess::DIAGONAL:
      {
         // Moving up and right
         if ( (finishingPos.iRow > startingPos.iRow) && (finishingPos.iColumn > startingPos.iColumn) )
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if (isSquareOccupied(startingPos.iRow + i, startingPos.iColumn + i))
               {
                  bFree = false;
                  //printf("Diagonal path up-right is not clear!\n");
               }
            }
         }

         // Moving up and left
         else if ( (finishingPos.iRow > startingPos.iRow) && (finishingPos.iColumn < startingPos.iColumn) )
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if (isSquareOccupied(startingPos.iRow+i, startingPos.iColumn-i))
               {
                  bFree = false;
                  //printf("Diagonal path up-left is not clear!\n");
               }
            }
         }

         // Moving down and right
         else if ( (finishingPos.iRow < startingPos.iRow) && (finishingPos.iColumn > startingPos.iColumn) )
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if (isSquareOccupied(startingPos.iRow - i, startingPos.iColumn + i))
               {
                  bFree = false;
                  //printf("Diagonal path down-right is not clear!\n");
               }
            }
         }

         // Moving down and left
         else if ( (finishingPos.iRow < startingPos.iRow) && (finishingPos.iColumn < startingPos.iColumn) )
         {
            // Settting bFree as initially true, only inside the cases, guarantees that the path is checked
            bFree = true;

            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if (isSquareOccupied(startingPos.iRow - i, startingPos.iColumn - i))
               {
                  bFree = false;
                  //printf("Diagonal path down-left is not clear!\n");
               }
            }
         }

         else
         {
            //m_reporter.ReportError("Error. Diagonal move not allowed");
         }
      }
      break;
   }

   return bFree;
}

bool Game::canBeBlocked(Position startingPos, Position finishingPos, int iDirection)
{
   bool bBlocked = false;

   Chess::UnderAttack blocker = {0};

   switch(iDirection)
   {
      case Chess::HORIZONTAL:
      {
         // If it is a horizontal move, we can assume the startingPos.iRow == finishingPos.iRow
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iColumn == finishingPos.iColumn)
         {
            //printf("Error. Movement is horizontal but column is the same\n");
         }

         // Moving to the right
         else if (startingPos.iColumn < finishingPos.iColumn)
         {
            for (int i = startingPos.iColumn + 1; i < finishingPos.iColumn; i++)
            {
               if ( true == isReachable( startingPos.iRow, i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         // Moving to the left
         else //if (startingPos.iColumn > finishingPos.iColumn)
         {
            for (int i = startingPos.iColumn - 1; i > finishingPos.iColumn; i--)
            {
               if ( true == isReachable( startingPos.iRow, i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }
      }
      break;

      case Chess::VERTICAL:
      {
         // If it is a vertical move, we can assume the startingPos.iColumn == finishingPos.iColumn
         // If the piece wants to move from column 0 to column 7, we must check if columns 1-6 are free
         if (startingPos.iRow == finishingPos.iRow)
         {
            m_reporter.ReportError("Error. Movement is vertical but row is the same\n");
         }

         // Moving up
         else if (startingPos.iRow < finishingPos.iRow)
         {
            for (int i = startingPos.iRow + 1; i < finishingPos.iRow; i++)
            {
               if ( true == isReachable( i, startingPos.iColumn, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         // Moving down
         else //if (startingPos.iColumn > finishingPos.iRow)
         {
            for (int i = startingPos.iRow - 1; i > finishingPos.iRow; i--)
            {
               if ( true == isReachable( i, startingPos.iColumn, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }
      }
      break;

      case Chess::DIAGONAL:
      {
         // Moving up and right
         if ( (finishingPos.iRow > startingPos.iRow) && (finishingPos.iColumn > startingPos.iColumn) )
         {
            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if ( true == isReachable( startingPos.iRow + i, startingPos.iColumn + i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         // Moving up and left
         else if ( (finishingPos.iRow > startingPos.iRow) && (finishingPos.iColumn < startingPos.iColumn) )
         {
            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if ( true == isReachable( startingPos.iRow + i, startingPos.iColumn - i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         // Moving down and right
         else if ( (finishingPos.iRow < startingPos.iRow) && (finishingPos.iColumn > startingPos.iColumn) )
         {
            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if ( true == isReachable( startingPos.iRow - i, startingPos.iColumn + i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         // Moving down and left
         else if ( (finishingPos.iRow < startingPos.iRow) && (finishingPos.iColumn < startingPos.iColumn) )
         {
            for (int i = 1; i < abs(finishingPos.iRow - startingPos.iRow); i++)
            {
               if ( true == isReachable( startingPos.iRow - i, startingPos.iColumn - i, getOpponentColor() ) )
               {
                  // Some piece can block the way
                  bBlocked = true;
               }
            }
         }

         else
         {
            m_reporter.ReportError("Error. Diagonal move not allowed\n");
         }
      }
      break;
   }

   return bBlocked;
}

bool Game::isCheckMate()
{
   bool bCheckmate = false;

   // 1. First of all, it the king in check?
   if ( false == playerKingInCheck() )
   {
      return false;
   }

   // 2. Can the king move the other square?
   Chess::Position king_moves[8]  = { {  1, -1 },{  1, 0 },{  1,  1 }, { 0,  1 },
                                      { -1,  1 },{ -1, 0 },{ -1, -1 }, { 0, -1 } };

   Chess::Position king = findKing(getCurrentTurn() );

   for (int i = 0; i < 8; i++)
   {
      int iRowToTest    = king.iRow    + king_moves[i].iRow;
      int iColumnToTest = king.iColumn + king_moves[i].iColumn;

      if ( iRowToTest < 0 || iRowToTest > 7 || iColumnToTest < 0 || iColumnToTest > 7 )
      {
         // This square does not even exist, so no need to test
         continue;
      }

      if ( EMPTY_SQUARE != getPieceAtPosition(iRowToTest, iColumnToTest) )
      {
         // That square is not empty, so no need to test
         continue;
      }

      Chess::IntendedMove intended_move;
      intended_move.chPiece      = getPieceAtPosition(king.iRow, king.iColumn);
      intended_move.from.iRow    = king.iRow;
      intended_move.from.iColumn = king.iColumn;
      intended_move.to.iRow      = iRowToTest;
      intended_move.to.iColumn   = iColumnToTest;

      // Now, for every possible move of the king, check if it would be in jeopardy
      // Since the move has already been made, current_game->getCurrentTurn() now will return
      // the next player's color. And it is in fact this king that we want to check for jeopardy
      Chess::UnderAttack king_moved = isUnderAttack( iRowToTest, iColumnToTest, getCurrentTurn(), &intended_move );

      if ( false == king_moved.bUnderAttack )
      {
         // This means there is at least one position when the king would not be in jeopardy, so that's not a checkmate
         return false;
      }
   }

   // 3. Can the attacker be taken or another piece get into the way?
   Chess::UnderAttack king_attacked = isUnderAttack( king.iRow, king.iColumn, getCurrentTurn() );
   if ( 1 == king_attacked.iNumAttackers )
   {
      // Can the attacker be taken?
      Chess::UnderAttack king_attacker = { 0 };
      king_attacker = isUnderAttack( king_attacked.attacker[0].pos.iRow, king_attacked.attacker[0].pos.iColumn, getOpponentColor() );

      if ( true == king_attacker.bUnderAttack )
      {
         // This means that the attacker can be taken, so it's not a checkmate
         return false;
      }
      else
      {
         // Last resort: can any piece get in between the attacker and the king?
         char chAttacker = getPieceAtPosition( king_attacked.attacker[0].pos.iRow, king_attacked.attacker[0].pos.iColumn );

         switch( toupper(chAttacker) )
         {
            case 'P':
            case 'N':
            {
               // If it's a pawn, there's no space in between the attacker and the king
               // If it's a knight, it doesn't matter because the knight can 'jump'
               // So, this is checkmate
               bCheckmate = true;
            }
            break;

            case 'B':
            {
               if ( false == canBeBlocked(king_attacked.attacker[0].pos, king, Chess::DIAGONAL ) )
               {
                  // If no piece can get in the way, it's a checkmate
                  bCheckmate = true;
               }
            }
            break;

            case 'R':
            {
               if ( false == canBeBlocked(king_attacked.attacker[0].pos, king, king_attacked.attacker[0].dir ) )
               {
                  // If no piece can get in the way, it's a checkmate
                  bCheckmate = true;
               }
            }
            break;

            case 'Q':
            {
               if ( false == canBeBlocked(king_attacked.attacker[0].pos, king, king_attacked.attacker[0].dir ) )
               {
                  // If no piece can get in the way, it's a checkmate
                  bCheckmate = true;
               }
            }
            break;


            default:
            {
               m_reporter.ReportError("!!!!Should not reach here. Invalid piece. In ischeckmate");
            }
            break;
         }
      }
   }
   else
   {
      // If there is more than one attacker and we reached this point, it's a checkmate
      bCheckmate      = true;
   }

   // If the game has ended, store in the class variable
   m_bGameFinished = bCheckmate;

   return bCheckmate;
}

bool Game::isOurPiece(char chPiece)
{
  return (getCurrentTurn() == Chess::WHITE_PLAYER) !=
         (islower(chPiece));
}

bool Game::isStaleMate()
{
   if ( true == playerKingInCheck() )
   // If the king is in check we aren't in stalemate
   {
      return false;
   }
   int iPieceRow, iPieceCol, i;
   // White pawns move down black ones move up
   int iPawnDirection = (getCurrentTurn() == Chess::WHITE_PLAYER ? +1 : -1);
   vector<Chess::Position> moves;
   Chess::EnPassant enpassant;
   Chess::Castling castling;
   Chess::Promotion promotion;
   Chess::PawnDoubleMove pawndoublemove;

   for (iPieceRow = 0; iPieceRow < 8; iPieceRow++)
   {
     for (iPieceCol = 0; iPieceCol < 8; iPieceCol++)
     {
       char chPiece = getPieceAtPosition(iPieceRow, iPieceCol);
       if ( EMPTY_SQUARE !=  chPiece)
       {
          if (isOurPiece(chPiece))
          // We can only move our own pieces
          {
            Chess::Position currentpos; 
            currentpos.iRow = iPieceRow;
            currentpos.iColumn = iPieceCol;
            switch(toupper(chPiece)){
              // Get a general list of all
              case 'P': // Pawn
                moves.push_back((Chess::Position){iPieceRow + iPawnDirection, iPieceCol - 1});
                moves.push_back((Chess::Position){iPieceRow + iPawnDirection, iPieceCol});
                moves.push_back((Chess::Position){iPieceRow + iPawnDirection, iPieceCol + 1});
                moves.push_back((Chess::Position){iPieceRow + 2 * iPawnDirection, iPieceCol});
              break;
              case 'R': // Rook
                for (i = 0; i < 8; i++) {
                  moves.push_back((Chess::Position){iPieceRow + i, iPieceCol});
                  moves.push_back((Chess::Position){iPieceRow - i, iPieceCol});
                  moves.push_back((Chess::Position){iPieceRow, iPieceCol + i});
                  moves.push_back((Chess::Position){iPieceRow, iPieceCol - i});
                }
              break;
              case 'B': // Bishop
                for (i = 0; i < 8; i++) {
                  moves.push_back((Chess::Position){iPieceRow + i, iPieceCol + i});
                  moves.push_back((Chess::Position){iPieceRow - i, iPieceCol - i});
                  moves.push_back((Chess::Position){iPieceRow - i, iPieceCol + i});
                  moves.push_back((Chess::Position){iPieceRow + i, iPieceCol - i});
                }
              break;
              case 'Q': // Queen
                for (i = 0; i < 8; i++) {
                  moves.push_back((Chess::Position){iPieceRow + i, iPieceCol});
                  moves.push_back((Chess::Position){iPieceRow - i, iPieceCol});
                  moves.push_back((Chess::Position){iPieceRow, iPieceCol + i});
                  moves.push_back((Chess::Position){iPieceRow, iPieceCol - i});
                  moves.push_back((Chess::Position){iPieceRow + i, iPieceCol + i});
                  moves.push_back((Chess::Position){iPieceRow - i, iPieceCol - i});
                  moves.push_back((Chess::Position){iPieceRow - i, iPieceCol + i});
                  moves.push_back((Chess::Position){iPieceRow + i, iPieceCol - i});
                }
              break;
              case 'K': // King
                for (i = 0; i < 2; i++) {
                  moves.push_back((Chess::Position){iPieceRow + i, iPieceCol});
                  moves.push_back((Chess::Position){iPieceRow - i, iPieceCol});
                  moves.push_back((Chess::Position){iPieceRow, iPieceCol + i});
                  moves.push_back((Chess::Position){iPieceRow, iPieceCol - i});
                  moves.push_back((Chess::Position){iPieceRow + i, iPieceCol + i});
                  moves.push_back((Chess::Position){iPieceRow - i, iPieceCol - i});
                  moves.push_back((Chess::Position){iPieceRow - i, iPieceCol + i});
                  moves.push_back((Chess::Position){iPieceRow + i, iPieceCol - i});
                }
              break;
            }
            for(const auto& targetpos : moves) {
              enpassant.bApplied = false;
              castling.bApplied = false;
              promotion.bApplied = false;
              if (targetpos.iRow >= 0 && targetpos.iRow < 8 &&
                  targetpos.iColumn >= 0 && targetpos.iColumn < 8 &&
                  isMoveValid(currentpos, targetpos, &enpassant, &castling,
                  &promotion, &pawndoublemove)) {
                    // If there are any valid moves we are not in stalemate
                    return false;
                  }
            }
          }
       }
     }

   }
   return true;
}

bool Game::isKingInCheck(int iColor, IntendedMove* pintended_move)
{
   bool bCheck = false;

   Position king = { 0 };

   // Must check if the intended move is to move the king itself
   if ( nullptr != pintended_move && 'K' == toupper( pintended_move->chPiece) )
   {
      king.iRow    = pintended_move->to.iRow;
      king.iColumn = pintended_move->to.iColumn;
   }
   else
   {
      king = findKing( iColor );
   }

   UnderAttack king_attacked = isUnderAttack( king.iRow, king.iColumn, iColor, pintended_move );

   if ( true == king_attacked.bUnderAttack )
   {
      bCheck = true;
   }

   return bCheck;
}

bool Game::playerKingInCheck(IntendedMove* intended_move)
{
   return isKingInCheck( getCurrentTurn(), intended_move);
}

bool Game::wouldKingBeInCheck(char chPiece, Position present, Position future, EnPassant* S_enPassant)
{
   IntendedMove intended_move;

   intended_move.chPiece      = chPiece;
   intended_move.from.iRow    = present.iRow;
   intended_move.from.iColumn = present.iColumn;
   intended_move.to.iRow      = future.iRow;
   intended_move.to.iColumn   = future.iColumn;

   return playerKingInCheck(&intended_move);
}

Chess::Position Game::findKing(int iColor)
{
   char chToLook = (WHITE_PIECE == iColor) ? 'K': 'k';
   Position king = { 0 };

   for (int i = 0; i < 8; i++)
   {
      for (int j = 0; j < 8; j++)
      {
         if ( chToLook == getPieceAtPosition(i, j) )
         {
            king.iRow    = i;
            king.iColumn = j;
         }
      }
   }

   return king;
}

void Game::changeTurns(void)
{
  m_round++;
}

bool Game::isFinished( void )
{
   return m_bGameFinished;
}

int Game::getCurrentTurn(void)
{
  if ((getCurrentRound() & 1) == 0) {
    return Chess::WHITE_PLAYER;
  } else {
    return Chess::BLACK_PLAYER;
  }
}

int Game::getCurrentRound(void)
{
  return m_round;
}

int Game::getOpponentColor(void)
{
   int iColor;

   if (Chess::WHITE_PLAYER == getCurrentTurn())
   {
      iColor = Chess::BLACK_PLAYER;
   }
   else
   {
      iColor = Chess::WHITE_PLAYER;
   }

   return iColor;
}

void Game::parseMove(std::string move, Position* pFrom, Position* pTo, char* chPromoted)
{
   pFrom->iColumn = move[0];
   pFrom->iRow    = move[1];
   pTo->iColumn   = move[3];
   pTo->iRow      = move[4];

   // Convert columns from ['A'-'H'] to [0x00-0x07]
   pFrom->iColumn = pFrom->iColumn - 'A';
   pTo->iColumn   = pTo->iColumn   - 'A';

   // Convert row from ['1'-'8'] to [0x00-0x07]
   pFrom->iRow  = pFrom->iRow  - '1';
   pTo->iRow    = pTo->iRow    - '1';

   if ( chPromoted != nullptr )
   {
      if ( move[5] == '=' )
      {
         *chPromoted = move[6];
      }
      else
      {
         *chPromoted = EMPTY_SQUARE;
      }
   }
}

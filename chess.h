#pragma once
#include "includes.h"

#define EMPTY_SQUARE 0x20

class IErrorReporter
// The original (Windows Console App) code printed error messages and threw
// exceptions. This is not compatible with EOS Smart Contracts. So the Smart
// Contract or other program called this library must implement this class
// to report error correctly when they occur
{
protected:
  IErrorReporter() { };

public:
  virtual void ReportError(std::string) = 0;

};

class Chess
{
public:
   static int getPieceColor( char chPiece );

   static bool isWhitePiece( char chPiece );

   static bool isBlackPiece( char chPiece );

   static std::string describePiece( char chPiece );

   enum PieceColor
   {
      WHITE_PIECE = 0,
      BLACK_PIECE = 1
   };

   enum Player
   {
      WHITE_PLAYER = 0,
      BLACK_PLAYER = 1
   };

   enum Side
   {
      QUEEN_SIDE = 2,
      KING_SIDE  = 3
   };

   enum Direction
   {
      HORIZONTAL = 0,
      VERTICAL,
      DIAGONAL,
      L_SHAPE
   };

   struct Position
   {
      int iRow;
      int iColumn;
   };

   struct EnPassant
   {
      bool bApplied;
      Position PawnCaptured;
   };

   struct Castling
   {
      bool bApplied;
      Position rook_before;
      Position rook_after;
   };

   struct Promotion
   {
      bool bApplied;
      //Position  pos;
      char chBefore;
      char chAfter;
   };

   struct IntendedMove
   {
      char chPiece;
      Position from;
      Position to;
   };

   struct Attacker
   {
      Position  pos;
      Direction dir;
   };

   struct UnderAttack
   {
      bool bUnderAttack;
      int iNumAttackers;
      Attacker attacker[9]; //maximum theorical number of attackers
   };

   /*
   const char initial_board[8][8] =
   {
      // This represents the pieces on the board.
      // Keep in mind that pieces[0][0] represents A1
      // pieces[1][1] represents B2 and so on.
      // Letters in CAPITAL are white
      { 'R',  'N',  'B',  'Q',  'K',  'B',  'N',  'R' },
      { 'P',  'P',  'P',  'P',  'P',  'P',  'P',  'P' },
      { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
      { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
      { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
      { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
      { 'p',  'p',  'p',  'p',  'p',  'p',  'p',  'p' },
      { 'r',  'n',  'b',  'q',  'k',  'b',  'n',  'r' },
   };
   */
};

class Game : Chess
{
public:
   Game(IErrorReporter& reporter, const char board[8][8], int round,
        Chess::Position LastMoveFrom, Chess::Position LastMoveTo);
   // reporter = an object that handles reporting errors
   // board = all the pieces
   // round = which round of the game this is. 0 is the first round
   // LastMoveFrom = Last move for the other player. The from position
   // LastMoveTo = Last move for the other player. The to position
   ~Game();

   bool isMoveValid(Chess::Position present, Chess::Position future, Chess::EnPassant* S_enPassant, Chess::Castling* S_castling, Chess::Promotion* S_promotion);

   void movePiece( Position present, Position future, Chess::EnPassant* S_enPassant, Chess::Castling* S_castling, Chess::Promotion* S_promotion );

   bool castlingAllowed( Side iSide, int iColor );

   char getPieceAtPosition( int iRow, int iColumn );

   char getPieceAtPosition( Position pos );

   char getPiece_considerMove( int iRow, int iColumn, IntendedMove* intended_move = nullptr );

   UnderAttack isUnderAttack( int iRow, int iColumn, int iColor, IntendedMove* pintended_move = nullptr );

   bool isReachable( int iRow, int iColumn, int iColor );

   bool isSquareOccupied( int iRow, int iColumn );

   bool isPathFree( Position startingPos, Position finishingPos, int iDirection );

   bool canBeBlocked( Position startingPos, Position finishinPos, int iDirection );

   bool isCheckMate();

   bool isStaleMate();

   bool isOurPiece(char chPiece);

   bool isKingInCheck( int iColor, IntendedMove* intended_move = nullptr );

   bool playerKingInCheck( IntendedMove* intended_move = nullptr );

   bool wouldKingBeInCheck( char chPiece, Position present, Position future, EnPassant* S_enPassant );

   Position findKing( int iColor );

   void changeTurns( void );

   bool isFinished( void );

   int getCurrentTurn( void );

   int getCurrentRound( void );

   int getOpponentColor( void );

   void parseMove( std::string move, Position* pFrom, Position* pTo, char* chPromoted = nullptr );

private:

   // Represent the pieces in the board
   char board[8][8];

   // Castling requirements
   bool m_bCastlingKingSideAllowed[2];
   bool m_bCastlingQueenSideAllowed[2];

   // Has the game finished already?
   bool m_bGameFinished;

   IErrorReporter& m_reporter;

   // Current turn number of the game. 0 = first turn
   int m_round;

   Chess::Position m_LastMoveFrom;
   Chess::Position m_LastMoveTo;
};

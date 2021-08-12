/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_OPCODE_H
#define FT_OPCODE_H

/*
 * Frame:
 *
 *   1. Bytecodes    byte
 *   2. Objects      object
 *   3. Names        string
 *   4. Offsets      int
 *   5. Types        type
 *
 * All bytecodes
 *
 *   J: Object   N: Name    F: Offset
 *   T: Type     P: Top     *: None
 */
typedef enum {
  CONST_OF,   // J
  LOAD_OF,    // N
  ENUMERATE,  // J
  CLASS,      // J
  FUNCTION,   // J
  LOAD_FACE,  // J
  ASSIGN_TO,  // N
  STORE_NAME, // N T
  TO_INDEX,   // *
  TO_REPLACE, // *
  GET_OF,     // N
  SET_OF,     // N
  CALL_FUNC,  // F
  ASS_ADD,    // N
  ASS_SUB,    // N
  ASS_MUL,    // N
  ASS_DIV,    // N
  ASS_SUR,    // N
  TO_REP_ADD, // *
  TO_REP_SUB, // *
  TO_REP_MUL, // *
  TO_REP_DIV, // *
  TO_REP_SUR, // *
  SE_ASS_ADD, // N
  SE_ASS_SUB, // N
  SE_ASS_MUL, // N
  SE_ASS_DIV, // N
  SE_ASS_SUR, // N
  SET_NAME,   // N
  NEW_OBJ,    // F
  USE_MOD,    // N
  BUILD_ARR,  // F
  BUILD_TUP,  // F
  BUILD_MAP,  // F
  TO_ADD,     // P1 + P2
  TO_SUB,     // P1 - P2
  TO_MUL,     // P1 * P2
  TO_DIV,     // P1 / P2
  TO_SUR,     // P1 % P2
  TO_GR,      // P1 > P2
  TO_LE,      // P1 < P2
  TO_GR_EQ,   // P1 >= P2
  TO_LE_EQ,   // P1 <= P2
  TO_EQ_EQ,   // P1 == P2
  TO_NOT_EQ,  // P1 != P2
  TO_AND,     // P1 & P2
  TO_OR,      // P1 | P2
  TO_BANG,    // !P
  TO_NOT,     // -P
  JUMP_TO,    // F
  T_JUMP_TO,  // F
  F_JUMP_TO,  // F
  TO_RET,     // *
  RET_OF,     // P
} op_code;

/* Output bytecode */
static const char *code_string[] = {
    "CONST_OF",   "LOAD_OF",    "ENUMERATE",  "CLASS",      "FUNCTION",
    "LOAD_FACE",  "ASSIGN_TO",  "STORE_NAME", "TO_INDEX",   "TO_REPLACE",
    "GET_OF",     "SET_OF",     "CALL_FUNC",  "ASS_ADD",    "ASS_SUB",
    "ASS_MUL",    "ASS_DIV",    "ASS_SUR",    "TO_REP_ADD", "TO_REP_SUB",
    "TO_REP_MUL", "TO_REP_DIV", "TO_REP_SUR", "SE_ASS_ADD", "SE_ASS_SUB",
    "SE_ASS_MUL", "SE_ASS_DIV", "SE_ASS_SUR", "SET_NAME",   "NEW_OBJ",
    "USE_MOD",    "BUILD_ARR",  "BUILD_TUP",  "BUILD_MAP",  "TO_ADD",
    "TO_SUB",     "TO_MUL",     "TO_DIV",     "TO_SUR",     "TO_GR",
    "TO_LE",      "TO_GR_EQ",   "TO_LE_EQ",   "TO_EQ_EQ",   "TO_NOT_EQ",
    "TO_AND",     "TO_OR",      "TO_BANG",    "TO_NOT",     "JUMP_TO",
    "T_JUMP_TO",  "F_JUMP_TO",  "TO_RET",     "RET_OF",
};

#endif
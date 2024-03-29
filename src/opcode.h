/* Drift
 *
 * 	- https://drift-lang.fun/
 *
 * GPL v3 License - bingxio <bingxio@qq.com> */
#ifndef FT_OPCODE_H
#define FT_OPCODE_H

typedef enum {
  CONST_OF,
  LOAD_OF,
  ENUMERATE,
  CLASS,
  FUNCTION,
  INTERFACE,
  ASSIGN_TO,
  STORE_NAME,
  TO_INDEX,
  TO_REPLACE,
  RANGE_OF,
  RANGE_GO,
  GET_OF,
  GET_IN_OF,
  SET_OF,
  CALL_FUNC,
  SET_EB,
  RECV_EB,
  SET_NAME,
  REF_MODULE,
  REF_SET,
  NEW_OBJ,
  USE_MOD,
  USE_IN_MOD,
  BUILD_ARR,
  BUILD_TUP,
  BUILD_MAP,
  TO_ADD,
  TO_SUB,
  TO_MUL,
  TO_DIV,
  TO_SUR,
  TO_GR,
  TO_LE,
  TO_GR_EQ,
  TO_LE_EQ,
  TO_EQ_EQ,
  TO_NOT_EQ,
  TO_AND,
  TO_OR,
  TO_BANG,
  TO_NOT,
  JUMP_TO,
  T_JUMP_TO,
  F_JUMP_TO,
  TO_RET,
  RET_OF,
} op_code;

static const char* code_string[] = {
    "CONST_OF",  "LOAD_OF",   "ENUMERATE",  "CLASS",      "FUNCTION",
    "INTERFACE", "ASSIGN_TO", "STORE_NAME", "TO_INDEX",   "TO_REPLACE",
    "RANGE_OF",  "RANGE_GO",  "GET_OF",     "GET_IN_OF",  "SET_OF",
    "CALL_FUNC", "SET_EB",    "RECV_EB",    "SET_NAME",   "REF_MODULE",
    "REF_SET",   "NEW_OBJ",   "USE_MOD",    "USE_IN_MOD", "BUILD_ARR",
    "BUILD_TUP", "BUILD_MAP", "TO_ADD",     "TO_SUB",     "TO_MUL",
    "TO_DIV",    "TO_SUR",    "TO_GR",      "TO_LE",      "TO_GR_EQ",
    "TO_LE_EQ",  "TO_EQ_EQ",  "TO_NOT_EQ",  "TO_AND",     "TO_OR",
    "TO_BANG",   "TO_NOT",    "JUMP_TO",    "T_JUMP_TO",  "F_JUMP_TO",
    "TO_RET",    "RET_OF",
};

#endif
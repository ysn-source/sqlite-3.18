/* Automatically generated.  Do not edit */
/* See the tool/mkopcodec.tcl script for details. */
#if !defined(SQLITE_OMIT_EXPLAIN) \
 || defined(VDBE_PROFILE) \
 || defined(SQLITE_DEBUG)
#if defined(SQLITE_ENABLE_EXPLAIN_COMMENTS) || defined(SQLITE_DEBUG)
# define OpHelp(X) "\0" X
#else
# define OpHelp(X)
#endif
const char *sqlite3OpcodeName(int i){
 static const char *const azName[] = {
    /*   0 */ "Savepoint"        OpHelp(""),
    /*   1 */ "AutoCommit"       OpHelp(""),
    /*   2 */ "Transaction"      OpHelp(""),
    /*   3 */ "SorterNext"       OpHelp(""),
    /*   4 */ "PrevIfOpen"       OpHelp(""),
    /*   5 */ "NextIfOpen"       OpHelp(""),
    /*   6 */ "Prev"             OpHelp(""),
    /*   7 */ "Next"             OpHelp(""),
    /*   8 */ "Checkpoint"       OpHelp(""),
    /*   9 */ "JournalMode"      OpHelp(""),
    /*  10 */ "Vacuum"           OpHelp(""),
    /*  11 */ "VFilter"          OpHelp("iplan=r[P3] zplan='P4'"),
    /*  12 */ "VUpdate"          OpHelp("data=r[P3@P2]"),
    /*  13 */ "Goto"             OpHelp(""),
    /*  14 */ "Gosub"            OpHelp(""),
    /*  15 */ "InitCoroutine"    OpHelp(""),
    /*  16 */ "Yield"            OpHelp(""),
    /*  17 */ "MustBeInt"        OpHelp(""),
    /*  18 */ "Jump"             OpHelp(""),
    /*  19 */ "Not"              OpHelp("r[P2]= !r[P1]"),
    /*  20 */ "Once"             OpHelp(""),
    /*  21 */ "If"               OpHelp(""),
    /*  22 */ "IfNot"            OpHelp(""),
    /*  23 */ "SeekLT"           OpHelp("key=r[P3@P4]"),
    /*  24 */ "SeekLE"           OpHelp("key=r[P3@P4]"),
    /*  25 */ "SeekGE"           OpHelp("key=r[P3@P4]"),
    /*  26 */ "SeekGT"           OpHelp("key=r[P3@P4]"),
    /*  27 */ "Or"               OpHelp("r[P3]=(r[P1] || r[P2])"),
    /*  28 */ "And"              OpHelp("r[P3]=(r[P1] && r[P2])"),
    /*  29 */ "NoConflict"       OpHelp("key=r[P3@P4]"),
    /*  30 */ "NotFound"         OpHelp("key=r[P3@P4]"),
    /*  31 */ "Found"            OpHelp("key=r[P3@P4]"),
    /*  32 */ "SeekRowid"        OpHelp("intkey=r[P3]"),
    /*  33 */ "NotExists"        OpHelp("intkey=r[P3]"),
    /*  34 */ "IsNull"           OpHelp("if r[P1]==NULL goto P2"),
    /*  35 */ "NotNull"          OpHelp("if r[P1]!=NULL goto P2"),
    /*  36 */ "Ne"               OpHelp("IF r[P3]!=r[P1]"),
    /*  37 */ "Eq"               OpHelp("IF r[P3]==r[P1]"),
    /*  38 */ "Gt"               OpHelp("IF r[P3]>r[P1]"),
    /*  39 */ "Le"               OpHelp("IF r[P3]<=r[P1]"),
    /*  40 */ "Lt"               OpHelp("IF r[P3]<r[P1]"),
    /*  41 */ "Ge"               OpHelp("IF r[P3]>=r[P1]"),
    /*  42 */ "ElseNotEq"        OpHelp(""),
    /*  43 */ "BitAnd"           OpHelp("r[P3]=r[P1]&r[P2]"),
    /*  44 */ "BitOr"            OpHelp("r[P3]=r[P1]|r[P2]"),
    /*  45 */ "ShiftLeft"        OpHelp("r[P3]=r[P2]<<r[P1]"),
    /*  46 */ "ShiftRight"       OpHelp("r[P3]=r[P2]>>r[P1]"),
    /*  47 */ "Add"              OpHelp("r[P3]=r[P1]+r[P2]"),
    /*  48 */ "Subtract"         OpHelp("r[P3]=r[P2]-r[P1]"),
    /*  49 */ "Multiply"         OpHelp("r[P3]=r[P1]*r[P2]"),
    /*  50 */ "Divide"           OpHelp("r[P3]=r[P2]/r[P1]"),
    /*  51 */ "Remainder"        OpHelp("r[P3]=r[P2]%r[P1]"),
    /*  52 */ "Concat"           OpHelp("r[P3]=r[P2]+r[P1]"),
    /*  53 */ "Last"             OpHelp(""),
    /*  54 */ "BitNot"           OpHelp("r[P1]= ~r[P1]"),
    /*  55 */ "IfSmaller"        OpHelp(""),
    /*  56 */ "SorterSort"       OpHelp(""),
    /*  57 */ "Sort"             OpHelp(""),
    /*  58 */ "Rewind"           OpHelp(""),
    /*  59 */ "IdxLE"            OpHelp("key=r[P3@P4]"),
    /*  60 */ "IdxGT"            OpHelp("key=r[P3@P4]"),
    /*  61 */ "IdxLT"            OpHelp("key=r[P3@P4]"),
    /*  62 */ "IdxGE"            OpHelp("key=r[P3@P4]"),
    /*  63 */ "RowSetRead"       OpHelp("r[P3]=rowset(P1)"),
    /*  64 */ "RowSetTest"       OpHelp("if r[P3] in rowset(P1) goto P2"),
    /*  65 */ "Program"          OpHelp(""),
    /*  66 */ "FkIfZero"         OpHelp("if fkctr[P1]==0 goto P2"),
    /*  67 */ "IfPos"            OpHelp("if r[P1]>0 then r[P1]-=P3, goto P2"),
    /*  68 */ "IfNotZero"        OpHelp("if r[P1]!=0 then r[P1]--, goto P2"),
    /*  69 */ "DecrJumpZero"     OpHelp("if (--r[P1])==0 goto P2"),
    /*  70 */ "IncrVacuum"       OpHelp(""),
    /*  71 */ "VNext"            OpHelp(""),
    /*  72 */ "Init"             OpHelp("Start at P2"),
    /*  73 */ "Return"           OpHelp(""),
    /*  74 */ "EndCoroutine"     OpHelp(""),
    /*  75 */ "HaltIfNull"       OpHelp("if r[P3]=null halt"),
    /*  76 */ "Halt"             OpHelp(""),
    /*  77 */ "Integer"          OpHelp("r[P2]=P1"),
    /*  78 */ "Int64"            OpHelp("r[P2]=P4"),
    /*  79 */ "String"           OpHelp("r[P2]='P4' (len=P1)"),
    /*  80 */ "Null"             OpHelp("r[P2..P3]=NULL"),
    /*  81 */ "SoftNull"         OpHelp("r[P1]=NULL"),
    /*  82 */ "Blob"             OpHelp("r[P2]=P4 (len=P1)"),
    /*  83 */ "Variable"         OpHelp("r[P2]=parameter(P1,P4)"),
    /*  84 */ "Into"             OpHelp("r[P2]=parameter(P1,P4)"),
    /*  85 */ "Define"           OpHelp(""),
    /*  86 */ "Move"             OpHelp("r[P2@P3]=r[P1@P3]"),
    /*  87 */ "Copy"             OpHelp("r[P2@P3+1]=r[P1@P3+1]"),
    /*  88 */ "SCopy"            OpHelp("r[P2]=r[P1]"),
    /*  89 */ "IntCopy"          OpHelp("r[P2]=r[P1]"),
    /*  90 */ "ResultRow"        OpHelp("output=r[P1@P2]"),
    /*  91 */ "CollSeq"          OpHelp(""),
    /*  92 */ "Function0"        OpHelp("r[P3]=func(r[P2@P5])"),
    /*  93 */ "Function"         OpHelp("r[P3]=func(r[P2@P5])"),
    /*  94 */ "AddImm"           OpHelp("r[P1]=r[P1]+P2"),
    /*  95 */ "RealAffinity"     OpHelp(""),
    /*  96 */ "Cast"             OpHelp("affinity(r[P1])"),
    /*  97 */ "Permutation"      OpHelp(""),
    /*  98 */ "String8"          OpHelp("r[P2]='P4'"),
    /*  99 */ "Compare"          OpHelp("r[P1@P3] <-> r[P2@P3]"),
    /* 100 */ "SeekSeq"          OpHelp(""),
    /* 101 */ "Win_Function"     OpHelp(""),
    /* 102 */ "Column"           OpHelp("r[P3]=PX"),
    /* 103 */ "Affinity"         OpHelp("affinity(r[P1@P2])"),
    /* 104 */ "MakeRecord"       OpHelp("r[P3]=mkrec(r[P1@P2])"),
    /* 105 */ "Count"            OpHelp("r[P2]=count()"),
    /* 106 */ "ReadCookie"       OpHelp(""),
    /* 107 */ "SetCookie"        OpHelp(""),
    /* 108 */ "ReopenIdx"        OpHelp("root=P2 iDb=P3"),
    /* 109 */ "OpenRead"         OpHelp("root=P2 iDb=P3"),
    /* 110 */ "OpenWrite"        OpHelp("root=P2 iDb=P3"),
    /* 111 */ "OpenAutoindex"    OpHelp("nColumn=P2"),
    /* 112 */ "OpenEphemeral"    OpHelp("nColumn=P2"),
    /* 113 */ "SorterOpen"       OpHelp(""),
    /* 114 */ "SequenceTest"     OpHelp("if( cursor[P1].ctr++ ) pc = P2"),
    /* 115 */ "OpenPseudo"       OpHelp("P3 columns in r[P2]"),
    /* 116 */ "Close"            OpHelp(""),
    /* 117 */ "ColumnsUsed"      OpHelp(""),
    /* 118 */ "Sequence"         OpHelp("r[P2]=cursor[P1].ctr++"),
    /* 119 */ "NewRowid"         OpHelp("r[P2]=rowid"),
    /* 120 */ "Insert"           OpHelp("intkey=r[P3] data=r[P2]"),
    /* 121 */ "InsertInt"        OpHelp("intkey=P3 data=r[P2]"),
    /* 122 */ "Delete"           OpHelp(""),
    /* 123 */ "ResetCount"       OpHelp(""),
    /* 124 */ "SorterCompare"    OpHelp("if key(P1)!=trim(r[P3],P4) goto P2"),
    /* 125 */ "SorterData"       OpHelp("r[P2]=data"),
    /* 126 */ "RowData"          OpHelp("r[P2]=data"),
    /* 127 */ "Rowid"            OpHelp("r[P2]=rowid"),
    /* 128 */ "NullRow"          OpHelp(""),
    /* 129 */ "SorterInsert"     OpHelp("key=r[P2]"),
    /* 130 */ "IdxInsert"        OpHelp("key=r[P2]"),
    /* 131 */ "IdxDelete"        OpHelp("key=r[P2@P3]"),
    /* 132 */ "Seek"             OpHelp("Move P3 to P1.rowid"),
    /* 133 */ "IdxRowid"         OpHelp("r[P2]=rowid"),
    /* 134 */ "Destroy"          OpHelp(""),
    /* 135 */ "Clear"            OpHelp(""),
    /* 136 */ "ResetSorter"      OpHelp(""),
    /* 137 */ "CreateIndex"      OpHelp("r[P2]=root iDb=P1"),
    /* 138 */ "Real"             OpHelp("r[P2]=P4"),
    /* 139 */ "CreateTable"      OpHelp("r[P2]=root iDb=P1"),
    /* 140 */ "SqlExec"          OpHelp(""),
    /* 141 */ "ParseSchema"      OpHelp(""),
    /* 142 */ "LoadAnalysis"     OpHelp(""),
    /* 143 */ "DropTable"        OpHelp(""),
    /* 144 */ "DropIndex"        OpHelp(""),
    /* 145 */ "DropTrigger"      OpHelp(""),
    /* 146 */ "DropSP"           OpHelp(""),
    /* 147 */ "IntegrityCk"      OpHelp(""),
    /* 148 */ "RowSetAdd"        OpHelp("rowset(P1)=r[P2]"),
    /* 149 */ "Param"            OpHelp(""),
    /* 150 */ "FkCounter"        OpHelp("fkctr[P1]+=P2"),
    /* 151 */ "MemMax"           OpHelp("r[P1]=max(r[P1],r[P2])"),
    /* 152 */ "OffsetLimit"      OpHelp("if r[P1]>0 then r[P2]=r[P1]+max(0,r[P3]) else r[P2]=(-1)"),
    /* 153 */ "WinStep0"         OpHelp("accum=r[P3] step(r[P2@P5])"),
    /* 154 */ "WinStep"          OpHelp("accum=r[P3] step(r[P2@P5])"),
    /* 155 */ "AggStep0"         OpHelp("accum=r[P3] step(r[P2@P5])"),
    /* 156 */ "AggStep"          OpHelp("accum=r[P3] step(r[P2@P5])"),
    /* 157 */ "AggFinal"         OpHelp("accum=r[P1] N=P2"),
    /* 158 */ "WinFinal"         OpHelp("accum=r[P1] N=P2"),
    /* 159 */ "Expire"           OpHelp(""),
    /* 160 */ "TableLock"        OpHelp("iDb=P1 root=P2 write=P3"),
    /* 161 */ "VBegin"           OpHelp(""),
    /* 162 */ "VCreate"          OpHelp(""),
    /* 163 */ "VDestroy"         OpHelp(""),
    /* 164 */ "VOpen"            OpHelp(""),
    /* 165 */ "VColumn"          OpHelp("r[P3]=vcolumn(P2)"),
    /* 166 */ "VRename"          OpHelp(""),
    /* 167 */ "Pagecount"        OpHelp(""),
    /* 168 */ "MaxPgcnt"         OpHelp(""),
    /* 169 */ "CursorHint"       OpHelp(""),
    /* 170 */ "Noop"             OpHelp(""),
    /* 171 */ "Explain"          OpHelp(""),
  };
  return azName[i];
}
#endif

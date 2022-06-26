
int ModbusRequestToRespond( unsigned char * Question, int LgtQuestion, unsigned char * Response );
int GetModbusSlaveNbrVars( unsigned char FunctCode );
void SetVarFromModbusSlave( unsigned char FunctCode, int ModbusNum, int Value );
int GetVarForModbusSlave( unsigned char FunctCode, int ModbusNum );


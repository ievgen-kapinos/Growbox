#ifndef GB_StorageHelper_h
#define GB_StorageHelper_h

#include "Global.h"
#include "Storage.h"
#include "StorageModel.h"

#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

//extern BootRecord bootRecord; // avoid craetion "cpp" file 

class GB_StorageHelper{

private:
  static BootRecord bootRecord;

public:

  static boolean load(){
    GB_Storage::read(0, &bootRecord, sizeof(BootRecord));
    if (isBootRecordCorrect()){
      bootRecord.lastStartupTimeStamp = now();
      GB_Storage::write(OFFSETOF(BootRecord, lastStartupTimeStamp), &(bootRecord.lastStartupTimeStamp), sizeof(bootRecord.lastStartupTimeStamp)
        );
      return true;   
    } 
    else {
      init();
      return false; 
    }
  }

  static boolean isLogRecordsOverflow(){
    return bootRecord.boolPreferencies.isLogOverflow;
  }

  static word getLogRecordsCapacity(){
    return (GB_Storage::CAPACITY - sizeof(BootRecord))/sizeof(LogRecord);
  }
  static word getLogRecordsCount(){
    if (bootRecord.boolPreferencies.isLogOverflow){
      return getLogRecordsCapacity(); 
    } 
    else {
      return (bootRecord.nextLogRecordAddress - sizeof(BootRecord))/sizeof(LogRecord);
    }
  }
  static boolean getLogRecord(word index, LogRecord &logRecord){
    if (index >= getLogRecordsCount()){
      return false;
    }

    word address = index * sizeof(LogRecord);
    if (bootRecord.boolPreferencies.isLogOverflow){
      address += bootRecord.nextLogRecordAddress;
    }

    word maxLogRecordAddress = sizeof(BootRecord) + getLogRecordsCapacity() * sizeof(LogRecord);

    if (address > maxLogRecordAddress){
      address -= maxLogRecordAddress;
    }

    GB_Storage::read(address, &logRecord, sizeof(LogRecord));  
    return true;


  }
  static boolean storeLogRecord(LogRecord &logRecord){ 
    boolean storeLog = isBootRecordCorrect() && bootRecord.boolPreferencies.isLoggerEnabled && GB_Storage::isPresent(); // TODO check in another places

    if (!storeLog){
      return false;
    }
    GB_Storage::write(bootRecord.nextLogRecordAddress, &logRecord, sizeof(LogRecord));
    increaseLogPointer();

    return true;
  }

  /////////////////////////////////////////////////////////////////////
  //                        GROWBOX COMMANDS                         //
  /////////////////////////////////////////////////////////////////////

  static void setLoggerEnabled(boolean flag){
    bootRecord.boolPreferencies.isLoggerEnabled = flag;
    GB_Storage::write(OFFSETOF(BootRecord, boolPreferencies), &(bootRecord.boolPreferencies), sizeof(bootRecord.boolPreferencies)); 
  }
  static boolean isLoggerEnabled(){
    return bootRecord.boolPreferencies.isLoggerEnabled; 
  }

  static time_t getFirstStartupTimeStamp(){
    return bootRecord.firstStartupTimeStamp; 
  }
  static time_t getLastStartupTimeStamp(){
    return bootRecord.lastStartupTimeStamp; 
  }

  static void resetLog(){

    bootRecord.nextLogRecordAddress = sizeof(BootRecord);
    GB_Storage::write(OFFSETOF(BootRecord, nextLogRecordAddress), &(bootRecord.nextLogRecordAddress), sizeof(bootRecord.nextLogRecordAddress)); 

    bootRecord.boolPreferencies.isLogOverflow = false;
    GB_Storage::write(OFFSETOF(BootRecord, boolPreferencies), &(bootRecord.boolPreferencies), sizeof(bootRecord.boolPreferencies));
  }

  static void resetFirmware(){
    bootRecord.first_magic = 0;
    GB_Storage::write(0, &bootRecord, sizeof(BootRecord));
  }

  static BootRecord getBootRecord(){
    return bootRecord; // Creates copy of boot record //TODO check it
  }

private :

  static boolean isBootRecordCorrect(){ // TODO rename it
    return (bootRecord.first_magic == MAGIC_NUMBER) && (bootRecord.last_magic == MAGIC_NUMBER);
  }

  static void increaseLogPointer(){
    bootRecord.nextLogRecordAddress += sizeof(LogRecord);  
    if (bootRecord.nextLogRecordAddress+sizeof(LogRecord) >= GB_Storage::CAPACITY){
      bootRecord.nextLogRecordAddress = sizeof(BootRecord);
      if (!bootRecord.boolPreferencies.isLogOverflow){
        bootRecord.boolPreferencies.isLogOverflow = true;
        GB_Storage::write(OFFSETOF(BootRecord, boolPreferencies), &(bootRecord.boolPreferencies), sizeof(bootRecord.boolPreferencies)); 
      }
    }
    GB_Storage::write(OFFSETOF(BootRecord, nextLogRecordAddress), &(bootRecord.nextLogRecordAddress), sizeof(bootRecord.nextLogRecordAddress)); 
  }

  static void init(){
    bootRecord.first_magic = MAGIC_NUMBER;
    bootRecord.firstStartupTimeStamp = now();
    bootRecord.lastStartupTimeStamp = bootRecord.firstStartupTimeStamp;
    bootRecord.nextLogRecordAddress = sizeof(BootRecord);
    bootRecord.boolPreferencies.isLogOverflow = false;
    bootRecord.boolPreferencies.isLoggerEnabled = true;
    for(byte i=0; i<sizeof(bootRecord.reserved); i++){
      bootRecord.reserved[i] = 0;
    }
    bootRecord.last_magic = MAGIC_NUMBER;
    GB_Storage::write(0, &bootRecord, sizeof(BootRecord));
  }
};

#endif








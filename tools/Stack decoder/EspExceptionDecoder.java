/*
  Copyright (c) 2015 Hristo Gochkov (ficeto at ficeto dot com)
  Modified by Rushikesh Patel 2018 CLI version (https://github.com/luffykesh)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
import java.nio.file.Files;
import java.nio.file.Paths;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.*;
import java.util.regex.*;

public class EspExceptionDecoder{
  File tool;
  File elf;
  String exceptionFile;
  String exceptionText;
  String outputText;
  String outputFile;

  private static String[] exceptions = {
    "Illegal instruction",
    "SYSCALL instruction",
    "InstructionFetchError: Processor internal physical address or data error during instruction fetch",
    "LoadStoreError: Processor internal physical address or data error during load or store",
    "Level1Interrupt: Level-1 interrupt as indicated by set level-1 bits in the INTERRUPT register",
    "Alloca: MOVSP instruction, if caller's registers are not in the register file",
    "IntegerDivideByZero: QUOS, QUOU, REMS, or REMU divisor operand is zero",
    "reserved",
    "Privileged: Attempt to execute a privileged operation when CRING ? 0",
    "LoadStoreAlignmentCause: Load or store to an unaligned address",
    "reserved",
    "reserved",
    "InstrPIFDataError: PIF data error during instruction fetch",
    "LoadStorePIFDataError: Synchronous PIF data error during LoadStore access",
    "InstrPIFAddrError: PIF address error during instruction fetch",
    "LoadStorePIFAddrError: Synchronous PIF address error during LoadStore access",
    "InstTLBMiss: Error during Instruction TLB refill",
    "InstTLBMultiHit: Multiple instruction TLB entries matched",
    "InstFetchPrivilege: An instruction fetch referenced a virtual address at a ring level less than CRING",
    "reserved",
    "InstFetchProhibited: An instruction fetch referenced a page mapped with an attribute that does not permit instruction fetch",
    "reserved",
    "reserved",
    "reserved",
    "LoadStoreTLBMiss: Error during TLB refill for a load or store",
    "LoadStoreTLBMultiHit: Multiple TLB entries matched for a load or store",
    "LoadStorePrivilege: A load or store referenced a virtual address at a ring level less than CRING",
    "reserved",
    "LoadProhibited: A load referenced a page mapped with an attribute that does not permit loads",
    "StoreProhibited: A store referenced a page mapped with an attribute that does not permit stores"
  };
  
  public static void main(String[] args) {
    EspExceptionDecoder decoder = new EspExceptionDecoder();
    decoder.decode(args);
  }
  private void decode(String[] args)
  {
    parseCliArgs(args);
    init();
    runParser();
  }
  public static Process execRedirected(String[] command) throws IOException {
    ProcessBuilder pb;

    // No problems on linux and mac
    if (!System.getProperty("os.name").startsWith("Windows")) {
      pb = new ProcessBuilder(command);
    } else {
      // Brutal hack to workaround windows command line parsing.
      // http://stackoverflow.com/questions/5969724/java-runtime-exec-fails-to-escape-characters-properly
      // http://msdn.microsoft.com/en-us/library/a1y7w461.aspx
      // http://bugs.sun.com/view_bug.do?bug_id=6468220
      // http://bugs.sun.com/view_bug.do?bug_id=6518827
      String[] cmdLine = new String[command.length];
      for (int i = 0; i < command.length; i++)
        cmdLine[i] = command[i].replace("\"", "\\\"");
      pb = new ProcessBuilder(cmdLine);
      Map<String, String> env = pb.environment();
      env.put("CYGWIN", "nodosfilewarning");
    }
    pb.redirectErrorStream(true);

    return pb.start();
  }

  private int listenOnProcess(String[] arguments){
    try {
      final Process p = execRedirected(arguments);
      Thread thread = new Thread() {
        public void run() {
          try {
            InputStreamReader reader = new InputStreamReader(p.getInputStream());
            int c;
            String line = "";
            while ((c = reader.read()) != -1){
              if((char)c == '\r')
                continue;
              if((char)c == '\n'){
                printLine(line);
                line = "";
              } else {
                line += (char)c;
              }
            }
            printLine(line);
            reader.close();

            reader = new InputStreamReader(p.getErrorStream());
            while ((c = reader.read()) != -1)
                System.err.print((char) c);
            reader.close();
          } catch (Exception e){}
        }
      };
      thread.start();
      int res = p.waitFor();
      thread.join();
      return res;
    } catch (Exception e){}
    return -1;
  }

  private void sysExec(final String[] arguments){
    Thread thread = new Thread() {
      public void run() {
        try {
          if(listenOnProcess(arguments) != 0){
            System.out.println("Decode Failed");
          } else {
            if(outputFile==null)
            {
              System.out.println("Decode Success");
              System.out.println(outputText);
            }
            else
            {
              try{
                Files.write(Paths.get(outputFile),outputText.getBytes());
              }
              catch(Exception e) {
                System.err.println("Error writing to: "+ outputFile);
                System.err.println(e.getMessage());
              }
            }
          }
        } catch (Exception e){
          System.err.println("Decode Exception");
          System.err.println(e.getMessage());
        }
      }
    };
    thread.start();
  }
  private void parseCliArgs(String[] args)
  {
    for(int i = 0 ;i<args.length;++i)
    {
      if(args[i].equals("-e"))
      {
        elf = new File(args[i+1]);
        ++i;
      }
      else if(args[i].equals("-g"))
      {
        tool = new File(args[i+1]);
        ++i;
      }
      else if(args[i].equals("-x"))
      {
        exceptionFile = args[i+1];
        ++i;
      }
      else if(args[i].equals("-o"))
      {
        outputFile = args[i+1];
        ++i;
      }
    }
  }

  private void init(){
    if(elf==null && System.getenv("ELF_FILE")!=null)
      elf = new File(System.getenv("ELF_FILE"));

    if(tool == null && System.getenv("XTENSA_GDB")!=null)
      tool = new File(System.getenv("XTENSA_GDB"));
    
    if(exceptionFile==null && System.getenv("EXP_FILE")!=null)
      exceptionFile = System.getenv("EXP_FILE");

    if(outputFile==null && System.getenv("DECODE_FILE")!=null)
      outputFile = System.getenv("DECODE_FILE");

    //default home directory
    if(exceptionFile==null)
      exceptionFile = System.getProperty("user.home") + File.separator + "exception.txt";


    if (tool==null || (!tool.exists() || !tool.isFile())) {
      System.err.println("GDB not found");
      System.exit(2);
    }
    if (elf==null || (!elf.exists() || !elf.isFile())) {
      System.err.println("ELF file not found");
      System.exit(3);
    }
    if(exceptionFile==null && !Files.exists(Paths.get(exceptionFile)))
    {
      System.err.println("Exception file not found");
      System.exit(4);
    }
    try
    {
      exceptionText = new String(Files.readAllBytes(Paths.get(exceptionFile)));
    }
    catch(IOException e)
    {
      System.err.println("Cannot read Exception file: "+ exceptionFile);
      System.exit(1);
    }
  }

  private String prettyPrintGDBLine(String line) {
    String address = "", method = "", file = "", fileline = "", html = "";

    if (!line.startsWith("0x")) {
      return null;
    }
  
    address = line.substring(0, line.indexOf(' '));
    line = line.substring(line.indexOf(' ') + 1);

    int atIndex = line.indexOf("is in ");
    if(atIndex == -1) {
      return null;
    }
    try {
        method = line.substring(atIndex + 6, line.lastIndexOf('(') - 1);
        fileline = line.substring(line.lastIndexOf('(') + 1, line.lastIndexOf(')'));
        file = fileline.substring(0, fileline.lastIndexOf(':'));
        line = fileline.substring(fileline.lastIndexOf(':') + 1);
        if(file.length() > 0){
          int lastfs = file.lastIndexOf('/');
          int lastbs = file.lastIndexOf('\\');
          int slash = (lastfs > lastbs)?lastfs:lastbs;
          if(slash != -1){
            String filename = file.substring(slash+1);
            file = file.substring(0,slash+1) + " " + filename + " ";
          }
        }
        html = address + ": " +
               method + " at " +
               file + " line " + line;
    } catch (Exception e) {
        // Something weird in the GDB output format, report what we can
        html = address + ": " + line;
    }

    return html;
  }

  private void printLine(String line){
    String s = prettyPrintGDBLine(line);
    if (s != null) 
      outputText += s +"\n";
  }

  private void parseException(){
    String content = exceptionText;
    Pattern p = Pattern.compile("Exception \\(([0-9]*)\\):");
    Matcher m = p.matcher(content);
    if(m.find()){
      int exception = Integer.parseInt(m.group(1));
      if(exception < 0 || exception > 29){
        return;
      }
      outputText += "Exception "+exception+": "+exceptions[exception]+"\n";
    }
  }

  // Strip out just the STACK lines or BACKTRACE line, and generate the reference log
  private void parseStackOrBacktrace(String regexp, boolean multiLine, String stripAfter) {
    String content = exceptionText;

    Pattern strip;
    if (multiLine) strip = Pattern.compile(regexp, Pattern.DOTALL);
    else strip = Pattern.compile(regexp);
    Matcher stripMatch = strip.matcher(content);
    if (!stripMatch.find()) {
      return; // Didn't find it in the text box.
    }

    // Strip out just the interesting bits to make RexExp sane
    content = content.substring(stripMatch.start(), stripMatch.end());

    if (stripAfter != null) {
      Pattern after = Pattern.compile(stripAfter);
      Matcher afterMatch = after.matcher(content);
      if (afterMatch.find()) {
          content = content.substring(0, afterMatch.start());
      }
    }

    // Anything looking like an instruction address, dump!
    Pattern p = Pattern.compile("40[0-2](\\d|[a-f]|[A-F]){5}\\b");
    int count = 0;
    Matcher m = p.matcher(content);
    while(m.find()) {
      count ++;
    }
    if(count == 0){
      return;
    }
    String command[] = new String[7 + count*2];
    int i = 0;
    command[i++] = tool.getAbsolutePath();
    command[i++] = "--batch";
    command[i++] = elf.getAbsolutePath();
    command[i++] = "-ex";
    command[i++] = "set listsize 1";
    m = p.matcher(content);
    while(m.find()) {
      command[i++] = "-ex";
      command[i++] = "l *0x"+content.substring(m.start(), m.end());
    }
    command[i++] = "-ex";
    command[i++] = "q";
    outputText += "\nDecoding stack results\n";
    sysExec(command);
  }

  // Heavyweight call GDB, run list on address, and return result if it succeeded
  private String decodeFunctionAtAddress( String addr ) {
    String command[] = new String[9];
    command[0] = tool.getAbsolutePath();
    command[1] = "--batch";
    command[2] = elf.getAbsolutePath();
    command[3] = "-ex";
    command[4] = "set listsize 1";
    command[5] = "-ex";
    command[6] = "l *0x" + addr;
    command[7] = "-ex";
    command[8] = "q";

    try {
      final Process proc = execRedirected(command);
      InputStreamReader reader = new InputStreamReader(proc.getInputStream());
      int c;
      String line = "";
      while ((c = reader.read()) != -1){
        if((char)c == '\r')
          continue;
        if((char)c == '\n' && line != ""){
          reader.close();
          return prettyPrintGDBLine(line);
        } else {
         line += (char)c;
        }
      }
      reader.close();
    } catch (Exception er) { }
    // Something went wrong
    return null;
  }

  // Scan and report the last failed memory allocation attempt, if present on the ESP8266
  private void parseAlloc() {
    String content = exceptionText;
    Pattern p = Pattern.compile("last failed alloc call: 40[0-2](\\d|[a-f]|[A-F]){5}\\((\\d)+\\)");
    Matcher m = p.matcher(content);
    if (m.find()) {
      String fs = content.substring(m.start(), m.end());
      Pattern p2 = Pattern.compile("40[0-2](\\d|[a-f]|[A-F]){5}\\b");
      Matcher m2 = p2.matcher(fs);
      if (m2.find()) {
        String addr = fs.substring(m2.start(), m2.end());
        Pattern p3 = Pattern.compile("\\((\\d)+\\)");
        Matcher m3 = p3.matcher(fs);
        if (m3.find()) {
          String size = fs.substring(m3.start()+1, m3.end()-1);
          String line = decodeFunctionAtAddress(addr);
          if (line != null) {
            outputText += "Memory allocation of " + size + " bytes failed at " + line + "\n";
          }
        }
      }
    }
  }

  // Filter out a register output given a regex (ESP8266/ESP32 differ in format)
  private void parseRegister(String regName, String prettyName) {
    String content = exceptionText;
    Pattern p = Pattern.compile(regName + "(\\d|[a-f]|[A-F]){8}\\b");
    Matcher m = p.matcher(content);
    if (m.find()) {
      String fs = content.substring(m.start(), m.end());
      Pattern p2 = Pattern.compile("(\\d|[a-f]|[A-F]){8}\\b");
      Matcher m2 = p2.matcher(fs);
      if (m2.find()) {
        String addr = fs.substring(m2.start(), m2.end());
        String line = decodeFunctionAtAddress(addr);
        if (line != null) {
          outputText += prettyName + ": " + line + "\n";
        } else {
          outputText += prettyName + ": 0x" + addr + "\n";
        }
      }
    }
  }

  //entry point
  private void runParser(){
    outputText = "";
    // Main error cause
    parseException();
    // ESP8266 register format
    parseRegister("epc1=0x", "PC");
    parseRegister("excvaddr=0x", "EXCVADDR");
    // ESP32 register format
    parseRegister("PC\\s*:\\s*(0x)?", "PC");
    parseRegister("EXCVADDR\\s*:\\s*(0x)?", "EXCVADDR");
    // Last memory allocation failure
    parseAlloc();
    // The stack on ESP8266, multiline
    parseStackOrBacktrace(">>>stack>>>(.)*", true, "<<<stack<<<");
    // The backtrace on ESP32, one-line only
    parseStackOrBacktrace("Backtrace:(.)*", false, null);
  }
}

///////////////////////////////////////////////////////////////////////////////
//                   SOFTWARE COPYRIGHT NOTICE AGREEMENT                     //
//       This software and its documentation are copyright (2010) by the     //
//   Broad Institute.  All rights are reserved.  This software is supplied   //
//   without any warranty or guaranteed support whatsoever. The Broad        //
//   Institute is not responsible for its use, misuse, or functionality.     //
///////////////////////////////////////////////////////////////////////////////

#ifndef PARSEDARGS
#define PARSEDARGS

/**
   Header: ParsedArgs.h

   Support for command-line argument parsing.

   This include file supports parsing of command-line arguments, according to the
   following scheme.  First, your "main program line" should be:
   
   > int main( int argc, char *argv[] )

   Second, to parse the arguments, do something like:
   
   > BeginCommandArguments;
   > CommandArgument_UnsignedInt(MAXT);
   > CommandArgument_String_OrDefault(TACO, "cheese");
   > EndCommandArguments;

   Here's what happens under various scenarios:

   (begin example)
   
   command line                   action
   
   a.out MAXT=5 TACO=beef         unsigned int MAXT = 5; String TACO = "beef";
   
   a.out MAXT=34                  unsigned int MAXT = 34; String TACO = "cheese";
   
   a.out TACO=broccoli            error because MAXT missing
   
   a.out MAXT=4 TACO=potato N=4   error because N is a superfluous argument

   (end example)

   If a variable name X is undefined on the command line, but an 
   environment variable 
   ARACHNE_X is defined, its value is used for X.  If X is undefined and ARACHNE_X
   is undefined in the environment, but FetchArgsFromFile is used, and X is 
   defined
   there, then we use that value.  Otherwise, if a default value is given, we use
   that.  Otherwise, an error is flagged.
   
   Special arguments of the form @@f, where f is a filename, cause extra arguments
   to be read from the file f.  Any line starting with # in this treated as a comment.

   Special Command-line Arguments that are always available:

   GDB (Bool) default: False
     Whether to use GDB for tracebacks.

   NO_HEADER, or NH (Bool) default: False 
     Whether to suppress the normal command-line header block.

   MEM_MONITOR, or MM (Bool) default: False 
     Whether or not to fork an instance of MemMonitor. All arguments specifiable
     to MemMonitor can be supplied here by prefixing them with '_MM_'.

   TEE (String)

     redirects cout to the specified list of files. Using this method instead of
     relying on unix redirect allows the Arachne module to detemine if its
     output has been redirected and to where. The Arachne module can then decide
     if it wants to turn on or off its own output redirection - for example in
     the case where a module creates a log file.

     TEE="file1 file2..."

   The CommandArgument macro defintions are auto generated by the perl script
   makeParsedArgsAuto.pl and can be found in system/ParsedArgsAuto.h.
   Do not edit this file manually.

   See Also: ParsedArgsAuto.h,  makeParsedArgsAuto.pl

   @file
*/

// When I have to do a return (even though the return statement can't be
// reached), I return GARBAGE.
#define GARBAGE 0
#define GARBAGESTRING String("Unreachable")

#include <fstream>

#include "String.h"
#include "system/Types.h"
#include "Vec.h"
#include "TokenizeString.h"
#include "ParseSet.h"

class parsed_arg_help {

 public:
  parsed_arg_help( const String& name, 
		   const String& type, 
		   const String& abbreviation,
		   const String& default_value,
		   const String& valid_values,
		   const String& documentation)
    : name_(name), 
      type_(type), 
      abbreviation_(abbreviation), 
      default_(default_value),
      valid_(valid_values),
      doc_(documentation)
  { }
  
  String GetName() const {
    return(name_);
  }

  bool operator< ( const parsed_arg_help& rhs ) const
  {
    return name_ < rhs.name_;
  }
  
  friend ostream& operator<< ( ostream& out, const parsed_arg_help& the_arg );
  bool isRequired() const {
    return default_ == "<required>";
  }

 private:
  String name_;
  String type_;
  String abbreviation_;
  String default_;
  String valid_;
  String doc_;
};

//namespace parsed_args_string {
  template<class T> inline String Stringize(T t) { return ToString(t); }
  template<> String inline Stringize<Bool>(Bool b) {
    return b ? "True" : "False";
  }
//};
    
///Contains all Arachne command-line parameters
class parsed_args {

  static const Bool INVALID_BOOL;
  static const int INVALID_INT;
  static const unsigned int INVALID_UINT;
  static const longlong INVALID_LONGLONG;
  static const double INVALID_DOUBLE;
  static const String INVALID_STRING;

  //using parsed_args_string::Stringize;

 public:

  static Bool pretty_help;
  static String usage_format;
  static String param_format;
  static String header_format;
  static String info_format;
  static String doc_format;
  static const parsed_args *ARGS;


  parsed_args( int argc, char *argv[], const Bool no_header = False );
  ~parsed_args();

  void FetchArgsFromFile( const String& filename );

  ///Put a command-line parameter and its value into this object.
  ///This method assumes that the parameter and its value are in the
  /// no-whitespace form "PARAM=Value"
  bool ParseString( const String& string );

  /// Put a command-line parameter and its value into this object.
  /// This method handles the case where the value is separated from the
  /// = sign by whitespace (e.g. "Param= Value"). 
  /// It is useful to be able to do this so as to 
  /// use tab completion in the shell.
  bool ParseString( int argc, char **argv, int & i);

  Bool GetHelpOnly( ) { return get_help_; }

  void SetHelpOnly(bool b) { get_help_ = b; }

  /// Displays error message when arg couldn't be converted to required type
  void ConversionFailure(int i);

  /// Displays error message when validation fails for default value
  void ValidationFailureDefault(const String& name, 
				const String& deflt,
				const String& valid );

  /// Displays error message when validation fails for ith arg
  void ValidationFailure(int i, const String& valid);

  void NoValue(String n);

  String CheckForArgAndAbbreviation( String n, String abbr );


  template<class T> 
    void GetValue (String n, T & value, const String & abbr = "", 
		   const String & deflt = "<required>",
		   const String & valid = "");

  template<class T>
    void ProcessArgs(T & value, const String & name,
		     const String & type, const String & abbr,
		     const String & def, const String & valid="",
		     const String & doc="") {
      if ( GetHelpOnly() )
	AddArgHelp( name, type, abbr, def, valid, doc );
      else
	GetValue( name, value, abbr, def, valid );
    }

  template<class T>
    void ProcessArgs(T & value, const String & name,
		     const String & type, const String & abbr,
		     const char * def, const String & valid="",
		     const String & doc="") {
    ProcessArgs(value, name, type, abbr, String(def), valid, doc);
  }


  template<class T>
    void ProcessArgs(T & value, const String & name,
		     const String & type, const String & abbr,
		     const double & def, const String & valid="",
		     const String & doc="") {
    stringstream s;
    s << def;
    String temp;
    s >> temp;
    ProcessArgs(value, name, type, abbr, temp, valid, doc);
  }

  template<class T>
    void ProcessArgs(T & value, const String & name,
		     const String & type, const String & abbr,
		     const float & def, const String & valid="",
		     const String & doc="") {
    stringstream s;
    s << def;
    String temp;
    s >> temp;
    ProcessArgs(value, name, type, abbr, temp, valid, doc);
  }

  template<class T, class S>
    void ProcessArgs(T & value, const String & name,
		     const String & type, const String & abbr,
		     S def, const String & valid="",
		     const String & doc="") {
      if ( GetHelpOnly() )
	AddArgHelp( name, type, abbr, Stringize(T(def)), valid, doc );
      else
	GetValue( name, value, abbr, Stringize(T(def)), valid );
    }


  String GetStringValue( String n, String abbr = "", 
			 String deflt = INVALID_STRING,
			 String valid = "");

  Bool GetBoolValue(String n, String abbr = "", Bool deflt = INVALID_BOOL );

  unsigned int GetUnsignedIntValue( String n, String abbr = "", 
				    unsigned int deflt = INVALID_UINT,
				    String valid = "" );

  int GetIntValue( String n, String abbr = "",
		   int deflt = INVALID_INT,
		   String valid = "");

  longlong GetLongLongValue( String n, String abbr = "",
			longlong deflt = INVALID_LONGLONG,
			String valid = "");

  double GetDoubleValue(String n, String abbr = "",
			double deflt = INVALID_DOUBLE,
			String valid = "");
 
  void CheckForExtraArgs( bool abort_if_found );

  void PrintVersion( );

  void AddArgHelp( const String& arg, 
                   const String& type, 
                   const String& abbreviation,
                   const String& default_value,
		   const String& valid_values="",
		   const String& documentation="");
     
  void PrintArgHelp( );

  String TheCommand( ) const;

  String GetProgramName( ) const { return command_; }

  void PrintTheCommandPretty( ostream& out, const String& prefix = "" );

  /// Removes command-line parameter from object
  bool RemoveArg(String n, String abbr = "");

  void AddDocString(const char *doc) { doc_ = doc; }

  void AddEnvVarPrefix(const String& prefix) { env_var_prefix_ = prefix; }


  vec<String> GetArgNames() const { return name_; }

  /// MethodDecl: GetOutputRedirection
  /// The special command-line argument TEE="file1 file2..."
  /// redirects cout to the specified files.  This method
  /// tells the caller the names of the files (if given)
  /// to which we are redirecting output, or the empty
  /// string if not redirecting.   The caller can then
  /// turn off their own redirection as needed.
  String GetOutputRedirection() const;

  void SetOutputRedirection(const String filenames);

 private:

  void CheckEnv( const String& n, const String& abbr );

  void MemMonitor();
  void ParseTheTee();
  void MakeTheTee(const String filenames);
  void RemoveTheTee();
  void PrintTheTee();

  String command_;
  vec<String> name_, def_;
  vec<Bool> used_;
  String orig_command_;
  Bool no_header_;
  String env_var_prefix_;


  Bool get_help_;
  String get_help_command_;
  vec<parsed_arg_help> arg_help_;

  const char * doc_;

  String outputRedirectedTo;
  procbuf *outputRedirectionPipe;
};  // class parsed_args

#define BeginCommandArguments \
     parsed_args command(argc, argv);

/// Add general description/documentation to module

#define CommandDoc(STRING) \
     command.AddDocString(STRING)

#define PrintCommandPretty(OUT) \
     command.PrintTheCommandPretty( OUT );

#define FetchCommandArgumentsFromDefaultsFile( defaults_file ) \
     command.FetchArgsFromFile( defaults_file );

// Set the environment variable prefix used to set command line arg values:
// PREFIX_ARGNAME=value
// Default is ARACHNE, e.g: ARACHNE_OUTPUT=out.txt

#define CommandEnvVarPrefix(STRING) \
     command.AddEnvVarPrefix(STRING)

// Import CommandArg macro defintions autogenerated by makeParsedArgsAuto.pl

#include "system/ParsedArgsAuto.h"

// Define extra VecString CommandArg macros for backwards compatibility.
// But please use CommandArgument_StringSet instead in any new code.

#define CommandArgument_VecString(NAME) \
     vec<String> NAME; \
     String NAME ## _COLON_CONCATENATED_LIST;\
     if ( command.GetHelpOnly() ) \
       command.AddArgHelp( #NAME, ":-sep strings", "", "<required>" ); \
     else { \
       NAME ## _COLON_CONCATENATED_LIST = command.GetStringValue( #NAME );\
       Tokenize(NAME ## _COLON_CONCATENATED_LIST, NAME, ":");\
     }

#define CommandArgument_VecString_OrDefault(NAME, DEFAULT) \
     vec<String> NAME; \
     String NAME ## _COLON_CONCATENATED_LIST;\
     if ( command.GetHelpOnly() ) \
       command.AddArgHelp( #NAME, ":-sep strings", "", DEFAULT ); \
     else { \
       NAME ## _COLON_CONCATENATED_LIST = command.GetStringValue( #NAME, "", DEFAULT );\
       Tokenize(NAME ## _COLON_CONCATENATED_LIST, NAME, ":");\
     }

#define EndCommandArguments \
     if ( command.GetHelpOnly() ) \
     { \
       command.PrintArgHelp( ); \
       exit(0); \
     } \
     else \
       command.CheckForExtraArgs( true );

#define EndCommandArgumentsAllowExtras \
     if ( command.GetHelpOnly() ) \
     { \
       command.PrintArgHelp( ); \
       exit(0); \
     } \
     else \
       command.CheckForExtraArgs( false );
     
///Use this macro if you want your program to run with no arguments.
///(instead of printing usage, that is). Mainly intended for unit tests.
#define BeginCommandArgumentsAcceptEmptyArgList \
BeginCommandArguments; if (argc == 1) { command.SetHelpOnly(false);} else{}

#define BeginCommandArgumentsAcceptEmptyArgListNoHeader \
     parsed_args command(argc, argv, True); \
     if (argc == 1) { command.SetHelpOnly(false);} else{}

#define BeginCommandArgumentsNoHeader \
     parsed_args command(argc, argv, True);

#endif

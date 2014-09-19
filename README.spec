Project specification
---------------------
"The goal is to write a primitive XML parser using only the standard
glibc library.

The program should:
* read in an xml file, 
* parse it into a data structure of your choosing, and
* print it to the screen.

If an error (such as a mis-matching close tag) is detected, the
program should:
* display an error,
* print the xml read in so far, and
* terminate

For the purposes of this assignment, there is no need to create a
validating parser, so you don't have to worry about DTDs or Schemas.

Additionally, the presence of processing instructions (ie. anything
between '<?' and '?>') and comments (ie. anything between '<!--' and
'-->') should be tolerated but their contents can be ignored."

> Do you want me to parse generic XML and print information about the XML
> itself, or invent a simple XML-based markup language and display it in a
> structured way?

"I'll be happy as long as it can be seen that you've correctly parsed the input."

With that description having quite a bit of leeway, this project defines a
subset of XML called Rudimentary Markup (RuM), a library for defining and
parsing arbitary languages conforming to RuM, a sample language,
and an application to display files written in the sample language.

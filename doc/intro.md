# Secure or secured software? A motivation for the SOFI security model

Computer security has been an important topic for several decades. Its
significance grows steadily as more and more activities are done online, using
computers, networks, and also embedded devices in the form of the internet of
things. In this text, I will focus on development of secure software. I will
present arguments that software security is often handled in an inherently
flawed way. I will suggest possible improvements, some more philosophical or
methodological, and another more practical and technical.

There are two complementary aspects of development of secure software. The
first one is a proper design and implementation of security related
functionality, for example, authentication and access control. The second
aspect is preventing security vulnerabilities in the complete code base, not
only in parts focused on security functions.

Security functions, as well as prevention of vulnerabilities, can be built from
the beginning of a software project, or added later, usually after creating at
least the first version of the core functionality of the software. There are
rational reasons for postponing development work related to security. A common
requirement is to have the first prototype as quickly as possible, and
postponing all non-essential parts helps to achieve this goal. Also, prototypes
and demos are usually run in well controlled non-production environment, so
they do not have to bother about security. If agile development is performed,
requirements regarding security may become well defined only after several
iterations of presenting the created software to users.

Unfortunately, although this approach often (but not always) works for adding
new features and fixing bugs in software, it tends to fail for implementing
security functions and removing security vulnerabilities. The key difference is
that buggy behavior is usually triggered more or less unintentionally and users
try to use workarounds actively. On the other way, vulnerabilities are often
exploited deliberately by a motivated and intelligent attacker. For this
reason, fixing vulnerabilities as they are discovered, or event actively
searching for them, is not efficient enough. Security-related functionality is
typically not localized in some isolated module, but it is spread across the
whole software. It has nontrivial implications to the program logic, so
security issues cannot be solved separately from the rest of the program.

Securing existing software is a desirable activity, but much safer and
efficient approach is to build software as secure from the very beginning. It
holds both on the conceptual level, where security functions should be
designed, and on the implementation level, using best practices for creating
secure code.

If all features related to security cannot be designed early, the system design
must be ready to add them later. As a minimum, an analysis should be performed
to ensure that securing the software would not require excessive redesign and
refactoring effort. If possible, hooks should be placed at strategic locations
in the code so that security modules can be plugged into them later. At least
some trivial mock security modules should be implemented to be sure that the
hooks are prepared correctly. Such an extensible design is advisable even when
the real security functionality is implemented immediately, because
requirements may change and additional functions may be needed.

The interface of security enforcing modules should as much as possible separate
enforcing security policies from the rest of program functions. Good examples
of this approach are MAC (Mandatory Access Control) frameworks and packet
filtering hooks in operating system kernels. Bad cases of mixing
overcomplicated ad-hoc security enforcement features with generic application
logic can be seen in web browsers and their same origin policy, permissions for
JavaScript code, cookie policies, certificate handling, etc.

On the low level of source code, a good coding standard should be enforced from
the beginning of implementation. Testing and code review should actively search
for vulnerabilities and violations of good coding practices. Third party
libraries and other components should be evaluated with regard to possible
security implications before adding them to the project. Such a precautionary
mindset is much more efficient in building secure software than reactive fixing
of security problems.

Up to now, I presented general principles of secure software development and
I argued that they are better than trying to ex post secure a piece of
software created without regard for security. In the rest of this text, I will
focus on one specific part of software security, namely authorization, that is,
defining rights to perform various operation.

Security models of widely used contemporary operating systems, like Windows or
Linux, are based on controlling access to system resources according to a user
identity. A user authenticates to the system and all processes executed in the
user session have the same permissions to use files and other resources.
A typical settings of access rights prevents a user to modify files owned by
other users or to damage the operating system installation, but any process
owned by a user has unrestricted access to files and processes owned by the
same user. This approach to operating system security was appropriate in the
era of UNIX minicomputers, when all software could be trusted, as it was
installed by the system administrator. A single computer was used by multiple
users and access rights were used to restrict access to data owned by different
users.

Today, a computer is often used by a single user, who runs untrusted and
potentially malicious programs. Preventing the user to read, write, or delete
files of other users does not increase security of the system, because there
are no other users. Protecting operating system files does not help much,
because the system can be easily recovered from installation media and all
important and sensitive data on the system are user data.

Some operating systems, for example Android, limit a potential harm a program
can do by using capabilities. For each application, the operating system
maintains a set of capabilities, which define the set of permitted operations
that the application can invoke. Each program should be assigned the minimum
set of capabilities necessary to function properly. This model of security
addresses threats of malicious applications, but it is not sufficient, for at
least two reasons. First, capabilities tend to be too coarse. For instance, an
application can be granted access to a memory card, but not to specific
directories or file types stored on the card. Second, the set of allowed
operations does not depend on data processed by the application. Hence, we
cannot specify that, for example, an installer may install only software
packages obtained from trusted sources, or a programming language
interpreter may not execute some functions if running a script received by
email.

It follows from the presented deficiencies of security models of operating
systems that a better model would combine permissions granted to users,
capabilities of programs, and tracking of data processed by a process. An
example of such tracking is the taint mode of programming language Perl. It
flags each individual value derived from outside the program (for example,
command line arguments and user input) and forbids using these values in some
operations, e.g., as a shell command, or in commands modifying files,
directories, or processes. If the programmer needs do an action otherwise
forbidden by taint checks, values involved must be explicitly untainted.

A security model should be built into the processing environment, which can be
an operating system, a scripting engine, or a web browser, and independent of
application program code. Let me present a sketch of a security model that
fulfils the above discussed criteria. It is called SOFI, which is an
abbreviation for Subjects and Objects with Floating Integrity. The model works
on two types of entities. Anything that actively invokes an operation is called
the subject. In an operating system, it is usually a process. The target of the
operation, typically a file, another process, a network socket, etc., is the
object. Each subject and object is assigned an integrity, which is an abstract
value describing the trust level of an entity. The key feature is that the
integrity values are floating. They are not fixed, but they are modified
according to a set of rules. An action performed by a subject on an object
usually includes transfer of some data, for example writing or reading a file.
The entity providing data is called the writer. The data receiving entity is
the reader. If the data transfer is bidirectional, the subject and the object
are both writers and readers simultaneously. Note that being a subject, an
object, a writer, or a reader are properties of each single operation. The same
entities can assume different roles in other operations.

An integrity value is a set of integrity attributes, which are single bit
elementary flags denoting qualities as: "owned by user X", "received from
a trusted source", or "tested as free of malware". The basic operations on
integrity values are therefore membership and subset tests, intersection, and
union. The subset relation defines a partial order of integrities.

The basic rules of the SOFI model check permissions to perform an operation and
modify integrities of involved entities. Each entity in the system has an
associated integrity, which is initialized to an appropriate value before the
first operation. Every potential object has an ACL (Access Control List), which
for each type of operation defines the minimum required integrity of the
subject.

If a subject wants to perform an operation on an object, the integrity of the
subject is tested, whether its value is at least as required by the ACL of the
object. If the test fails, the operation is denied. If the test succeeds, the
next step is integrity modification. The integrity of the reader is set to the
intersection of its original integrity and the integrity of the writer.
Finally, the operation is performed, including a data transfer.

These rules ensure that low-integrity entities cannot abuse privileges of
high-integrity entities to escalate its own privileges. For example, files
downloaded from an untrusted source are assigned a very low integrity by the
operating system. When such a files is open by a user, or executed (if it is
a program), it does not get full privileges of the user. Hence it cannot modify
other files owned by the user. Even if run by a superuser, the program will run
with limited privileges and hence it cannot damage important system files.
Similarly, executable files created by a user cannot access private data of
another user, even if executed by that user.

The basic integrity modification rules presented so far would tend to gradually
lower integrities of all entities of the system, which is not desirable. Hence
we add several more rules. 

In addition to the current integrity, which is dynamically adjusted whenever
an entity is the reader in some operation, each entity has also a minimum
allowed integrity. Whenever an operation would cause the current integrity
value to be decreased below this minimum, the operation is not executed and the
current integrity is not changed.

Each entity has a mask of integrity attributes that are not removed from the
integrity value even when the entity is a reader in some operation and the
writer does not posses these attributes. For example, an antivirus program,
which reads files with very low integrity, needs to retain the integrity high
enough to be able to write log files and delete or quarantine infected files.
Of course, care must be taken when using this feature, in order that a poorly
written application cannot compromise security of the whole system.

An entity, when acting as a writer, has also a mask of integrity attributes
that can be added to the integrity value of the reader entity. This feature can
be used to allow performing privileged operations by low-priority subjects in
a controlled way, somewhat similar to the setuid bit in UNIX. When a program is
executed by a process, the executable file is treated as the writer and the
process is the reader. For example, a password changing program can modify the
system password database, although it is run by an ordinary user, who does not
have rights to access the database. Another more complex example is checking of
integrity of software packages. If an ordinary user downloads a software
package, the package file is assigned so low integrity, that it cannot be
installed even by the system administrator. But the administrator can obtain
a cryptographic checksum or signature of the package from a trusted
distribution site and then invoke a signature checking program. If the
signature is successfully verified, the checker increases the integrity of the
package file, so that it can be installed now.

The SOFI security model, as presented so far, controls integrity and access
rights of entities in a system. It could be probably extended to maintain
confidentiality of data by adding rules preventing secret information flow to
untrusted (low integrity) destinations. For instance, a high-integrity
application for decrypting received encrypted email messages, although being
able to send normal data via network, would not be allowed to perform network
communication after reading user's secret key.

A natural platform for implementing the SOFI security model is an operating
system kernel, where it can amend or replace existing permission control
features. The model is generic, not limited to a specific types of computing
system entities or environment, therefore it can be used in virtually any
context. Other examples of kinds of software that can benefit from it are file
servers, databases, and multiuser information systems. A prominent use case 
is web browsers security. All various security policies, like the same origin
policy, permissions for JavaScript to access local system resources, or
limitations of accepted cookies, could be replaced by a distributed SOFI
implementation. It would combine rules defined locally by the browser with
rules sent by web servers in HTTP headers.

The previous paragraphs present the initial sketch of the SOFI security model.
To assess its potential to improve general security of software systems, it
would have to be defined more formally and then implemented. A good environment 
for the first implementation would be Linux or FreeBSD kernel, or a secure
browser.

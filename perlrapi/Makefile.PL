use ExtUtils::MakeMaker;
WriteMakefile('AUTHOR'       => 'Andreas Pohl <osar@users.sourceforge.net>',
	      'NAME'         => 'Rapi2',
	      'LIBS'         => ['-lrapi'],
	      'OBJECT'       => 'perlrapi_wrap.o',
	      'VERSION_FROM' => 'Rapi2.pm',
	      'DIR'          => [qw(Simple Defs)]
);

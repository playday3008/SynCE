use ExtUtils::MakeMaker;
WriteMakefile('AUTHOR'       => 'Andreas Pohl (d3@dimension3.de)',
	      'NAME'         => 'Rapi2',
	      'LIBS'         => ['-lrapi'],
	      'OBJECT'       => 'perlrapi_wrap.o',
	      'VERSION_FROM' => 'Rapi2.pm',
	      'DIR'          => [qw(Simple Defs)]
);

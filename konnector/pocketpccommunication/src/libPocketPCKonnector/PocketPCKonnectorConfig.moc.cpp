/****************************************************************************
** pocketPCHelper::PocketPCKonnectorConfig meta object code from reading C++ file 'PocketPCKonnectorConfig.h'
**
** Created: Mon Oct 18 16:44:55 2004
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "PocketPCKonnectorConfig.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.3.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *pocketPCHelper::PocketPCKonnectorConfig::className() const
{
    return "pocketPCHelper::PocketPCKonnectorConfig";
}

QMetaObject *pocketPCHelper::PocketPCKonnectorConfig::metaObj = 0;
static QMetaObjectCleanUp cleanUp_pocketPCHelper__PocketPCKonnectorConfig( "pocketPCHelper::PocketPCKonnectorConfig", &pocketPCHelper::PocketPCKonnectorConfig::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString pocketPCHelper::PocketPCKonnectorConfig::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "pocketPCHelper::PocketPCKonnectorConfig", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString pocketPCHelper::PocketPCKonnectorConfig::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "pocketPCHelper::PocketPCKonnectorConfig", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* pocketPCHelper::PocketPCKonnectorConfig::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = KRES::ConfigWidget::staticMetaObject();
    metaObj = QMetaObject::new_metaobject(
	"pocketPCHelper::PocketPCKonnectorConfig", parentObject,
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_pocketPCHelper__PocketPCKonnectorConfig.setMetaObject( metaObj );
    return metaObj;
}

void* pocketPCHelper::PocketPCKonnectorConfig::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "pocketPCHelper::PocketPCKonnectorConfig" ) )
	return this;
    return ConfigWidget::qt_cast( clname );
}

bool pocketPCHelper::PocketPCKonnectorConfig::qt_invoke( int _id, QUObject* _o )
{
    return ConfigWidget::qt_invoke(_id,_o);
}

bool pocketPCHelper::PocketPCKonnectorConfig::qt_emit( int _id, QUObject* _o )
{
    return ConfigWidget::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool pocketPCHelper::PocketPCKonnectorConfig::qt_property( int id, int f, QVariant* v)
{
    return ConfigWidget::qt_property( id, f, v);
}

bool pocketPCHelper::PocketPCKonnectorConfig::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES

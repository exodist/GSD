$( function() {
    $( '.nav' ).click( function() {
        var item = $(this)

        $('#subnav ul').empty();
        $('ul.second_subnav').detach();
        $('#view').empty();

        jQuery.ajax(
            item.attr( 'id' ) + '.html',
            {
                dataType: 'html',
                success: function( data ) {
                    $( '.nav' ).removeClass( 'active' )
                    item.addClass( 'active' )

                    build_content( data );
                    fixView();
                },
                error: function() {
                    $('#view').html( '<div class="error">Error loading page</div>' )
                    $( '.nav' ).removeClass( 'active' )
                    item.addClass( 'active' )
                    fixView();
                }
            }
        )
    });

    var hash = window.location.hash
    if ( hash ) {
        var page = hash.split('-');
        $( page[0] ).trigger( 'click' )
    }
    else {
        $( '#about' ).trigger( 'click' )
    }
})

function fixView() {
    var height = $('#subnav').outerHeight();
    if ( height < 350 ) height = 350;
    $('#view').css( 'min-height', height );
    console.log( $(this).outerHeight(), height );
}

function build_content( data ) {
    var view = $( '#view' );
    var subnav = $( 'ul#main_subnav' );

    var hash = window.location.hash;
    if ( !hash ) hash = '#about';
    var nav = hash.split('-');

    var new_stuff = $( '<div></div>' );
    new_stuff.html( data );
    $(new_stuff).find( 'dl.listnav' ).each( function() {
        $(this).children('dt').each( function() {
            var id = $(this).attr( 'id' );
            var dt = $(this);
            var dd = $(this).next();
            var navitem = $(
                '<li id="' + id + '"><a href="' + nav[0] + '-' + id + '">' + dt.html() + '</a></li>'
            );
            var viewitem = $(
                '<div style="display: none">' + dd.html() + '</div>'
            );

            process( id, viewitem );

            var normal_click = function() {
                view.children().hide();
                $('ul.second_subnav').hide();
                subnav.children().removeClass( 'active' );
                viewitem.show();
                var sn = $('ul#SN-' + id);
                if ( sn.length ) {
                    sn.show();
                    sn.children().removeClass('active');
                    subnav.children().hide();
                    navitem.unbind( 'click' );
                    navitem.click( function() {
                        subnav.children().show();
                        navitem.unbind( 'click' );
                        navitem.click( normal_click );
                        fixView();
                    });
                }
                navitem.addClass( 'active' );
                navitem.show();
                fixView();
            };

            navitem.click( normal_click );

            subnav.append( navitem );
            view.append( viewitem );
        });

        if ( nav[1] ) {
            $( '#' + nav[1] ).trigger( 'click' );
            $('ul#SN-' + nav[1]).each( function() {
                $(this).show();
                if ( nav[2] ) {
                    $(this).find( '#' + nav[2] ).trigger( 'click' );
                }
            });
        }
        else {
            subnav.children().first().trigger( 'click' );
        }
    })
}

function process( id, container ) {
    container.find( 'div.symbol_list' ).each( function() {
        var list = $(this);
        jQuery.ajax(
            list.attr( 'src' ),
            {
                dataType: 'json',
                success: function( data ) {
                    list.replaceWith( build_symbol_list( data ));
                    fixView();
                },
                error: function(blah, message1, message2) {
                    $('#view').append( '<div class="error">Error loading ' + list.attr( 'src' ) + '</div>' )
                    fixView();
                }
            }
        )
    });

    container.find( 'div.sub_list' ).each( function() {
        var list = $(this);
        jQuery.ajax(
            list.attr( 'src' ),
            {
                async: false,
                dataType: 'json',
                success: function( data ) {
                    list.detach();
                    build_sub_list( id, data );
                    fixView();
                },
                error: function(blah, message1, message2) {
                    $('#view').append( '<div class="error">Error loading ' + list.attr( 'src' ) + '</div>' )
                    fixView();
                }
            }
        )
    });
}

function build_sub_list( pid, data ) {
    var subnav = $( '<ul id="SN-' + pid + '" style="display: none;" class="second_subnav listnav"></ul>' );

    var hash = window.location.hash;
    if ( !hash ) hash = '#about';
    var nav = hash.split('-');

    for (navkey in data) {
        build_sub_list_item( pid, navkey, data, nav, subnav );
    }

    $("#subnav").append( subnav );
}

function build_sub_list_item( pid, navkey, data, nav, subnav ) {
    var navname    = data[navkey]["name"];
    if ( !navname ) navname = navkey;
    var desc       = data[navkey]["desc"];
    var roles      = data[navkey]["roles"];
    var attributes = data[navkey]["attributes"];
    var methods    = data[navkey]["methods"];
    var requires   = data[navkey]["requires"];
    var usage       = data[navkey]["usage"];

    var navitem = $(
        '<li id="' + navkey + '"><a href="' + nav[0] + '-' + pid + '-' + navkey + '">' + navname + '</a></li>'
    );
    var viewitem = $(
        '<div style="display: none"><h2>' + navname + '</h2>' + desc + '</div>'
    );

    if ( usage ) {
        viewitem.append( '<h3>Usage</h3>' );
        viewitem.append( usage );
    }

    if ( roles ) {
        viewitem.append( '<h3>Roles:</h3>' );
        var list = $('<ul class="roll_list"></ul>');
        for (role in roles) {
            list.append( '<li><a href="' + nav[0] + '-roles-' + roles[role] + '" onclick="openRole(\'' + roles[role] + '\')">' + roles[role] + '<a></li>' );
        }
        viewitem.append( list );
    }

    if ( requires ) {
        viewitem.append( '<h3>Required Methods:</h3>' );
        viewitem.append( build_symbol_list( requires ));
    }

    if ( attributes ) {
        viewitem.append( '<h3>Attributes:</h3>' );
        viewitem.append( build_symbol_list( attributes ));
    }

    if ( methods ) {
        viewitem.append( '<h3>Methods:</h3>' );
        viewitem.append( build_symbol_list( methods ));
    }

    navitem.click( function() {
        $('#view').children().hide();
        subnav.children().removeClass( 'active' );
        navitem.addClass( 'active' );
        viewitem.show();
        fixView();
    });

    subnav.append( navitem );
    $('#view').append( viewitem );
}

function build_symbol_list( data ) {
    var table = $( '<table class="symbol_list"><tbody><tr><th>Name</th><th>Description</th></tr></tbody></table>' );
    for (key in data) {
        var name = data[key]['name'];
        if ( !name ) name = key;
        var row = $( '<tr class="symbol" onclick="expandDesc(this)"></tr>' );
        row.append( $('<td class="left">' + name + '</td>') );
        row.append( $('<td class="right">' + data[key]['desc'] + '</td>') );

        var details = $( '<td colspan="2"></td>' );
        if ( data[key]['usage'] ) {
            var list = $( '<ul style="usage"></ul>' );
            for ( i in data[key]['usage'] ) {
                var item = $( '<li><span style="example">' + data[key]['usage'][i] + '</span></li>' );
                list.append( item );
            }
            details.append( list );
        }
        details.append( data[key]['details'] );

        var drow = $( '<tr class="symbol_details" style="display: none;"></tr>' );
        drow.append( details );

        table.append( row );
        table.append( drow );
    }

    return table;
}

function expandDesc( e ) {
    $(e).toggleClass( 'open' );
    $(e).next().toggle();
}

function openRole( role ) {
    $('#main_subnav').find('#roles').trigger( 'click' );
    $('#SN-roles').find('#' + role).trigger( 'click' );
    fixView();
}

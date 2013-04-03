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
                },
                error: function() {
                    $('#view').html( '<div class="error">Error loading page</div>' )
                    $( '.nav' ).removeClass( 'active' )
                    item.addClass( 'active' )
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

            navitem.click( function() {
                view.children().hide();
                $('ul.second_subnav').hide();
                subnav.children().removeClass( 'active' );
                navitem.addClass( 'active' );
                viewitem.show();
                $('ul#SN-' + id).show();
                $('ul#SN-' + id).children().removeClass('active');
            });

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
                },
                error: function(blah, message1, message2) {
                    $('#view').append( '<div class="error">Error loading ' + list.attr( 'src' ) + '</div>' )
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
                },
                error: function(blah, message1, message2) {
                    $('#view').append( '<div class="error">Error loading ' + list.attr( 'src' ) + '</div>' )
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
    var desc       = data[navkey]["desc"];
    var roles      = data[navkey]["roles"];
    var attributes = data[navkey]["attributes"];
    var methods    = data[navkey]["methods"];

    var navitem = $(
        '<li id="' + navkey + '"><a href="' + nav[0] + '-' + pid + '-' + navkey + '">' + navname + '</a></li>'
    );
    var viewitem = $(
        '<div style="display: none"><h2>' + navname + '</h2>' + desc + '</div>'
    );

    if ( roles ) {
        viewitem.append( '<h3>Roles:</h3>' );
        var list = $('<ul class="roll_list"></ul>');
        for (role in roles) {
            list.append( '<li>' + roles[role] + '</li>' );
        }
        viewitem.append( list );
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
    });

    subnav.append( navitem );
    $('#view').append( viewitem );
}

function build_symbol_list( data ) {
    var table = $( '<table class="symbol_list"><tbody><tr><th>Name</th><th>Description</th></tr></tbody></table>' );
    for (key in data) {
        var row = $( '<tr class="symbol" onclick="expandDesc(this)"></tr>' );
        row.append( $('<td class="left">' + data[key]['name'] + '</td>') );
        row.append( $('<td class="right">' + data[key]['desc'] + '</td>') );
        var drow = $( '<tr class="symbol_details" style="display: none;"><td colspan="2">' + data[key]['details'] + '</td>' );
        table.append( row );
        table.append( drow );
    }

    return table;
}

function expandDesc( e ) {
    $(e).toggleClass( 'open' );
    $(e).next().toggle();
}

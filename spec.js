$( function() {
    $( '.nav' ).click( function() {
        var item = $(this)
        jQuery.ajax(
            item.attr( 'id' ) + '.html',
            {
                dataType: 'html',
                success: function( data ) {
                    $('#content').html( data );
                    $( '.nav' ).removeClass( 'active' )
                    item.addClass( 'active' )

                    specs_fix_list_nav( $('#content') );
                },
                error: function() {
                    $('#content').html( '<div class="error">Error loading page</div>' )
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

function specs_fix_list_nav( container ) {
    var nav = $( '<ul class="listnav"></ul>' );
    var view = $( '<div class="listview"></div>' );

    var hash = window.location.hash;
    if ( !hash ) hash = 'about';
    var page = hash.split('-');

    container.find( 'dl.listnav' ).each( function() {
        $(this).children('dt').each( function() {
            var id = $(this).attr( 'id' );
            var dt = $(this);
            var dd = $(this).next()
            var navitem  = $(
                '<li id="' + id + '"><a href="' + page[0] + '-' + id + '">' + dt.html() + '</a></li>'
            );
            var viewitem = $(
                '<div style="display: none">' + dd.html() + '</div>'
            );

            navitem.click( function() {
                view.children().hide();
                nav.children().removeClass( 'active' );
                navitem.addClass( 'active' );
                viewitem.show();
            });

            nav.append( navitem );
            view.append( viewitem );
        });
        $(this).parent().append( nav );
        $(this).parent().append( view );
        $(this).detach();

        if ( page[1] ) {
            $( '#' + page[1] ).trigger( 'click' );
        }
        else {
            nav.children().first().trigger( 'click' );
        }
    })
}

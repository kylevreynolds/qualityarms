jQuery(document).ready(function(){
  
    jQuery(function() {
        jQuery('#webform-component-enter-your-email .form-text').focus(function() {
            if (jQuery(this).val() == 'Enter your email')
                jQuery(this).val('');
        });
     
        jQuery('#webform-component-enter-your-email .form-text').blur(function() {
            if(jQuery(this).val() == '')
                jQuery(this).val('Enter your email');
        });
    });
});

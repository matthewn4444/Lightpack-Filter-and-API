(function($) {
    $.widget("custom.switchButton", {
        options: {
            onText: "On",
            offText: "Off",
            checked: true,
        },
        
        _create: function() {
            this.element.addClass("switch-button")
                .append($("<div>").addClass("container")
                    .append($("<span>").text(this.options.onText))
                    .append($("<span>").addClass("separator"))
                    .append($("<span>").text(this.options.offText))
                );
            this.element.toggleClass("checked", this.options.checked);            

            // Click event
            var that = this;
            this.element.on("click", function(e) {
                var isChecked = that.options.checked = !that.options.checked;
                $(this).toggleClass("checked", isChecked);
                that._trigger("click", e, {checked: isChecked });
            });
        },
        
        _destroy: function() {
            this.element.removeClass("switch-button");
        },
        
        checked: function(flag) {
            if (!arguments.length) {
                return this.options.checked;
            } else {
                this.element.toggleClass("checked", flag);
                this.options.checked = flag;
            }
        },
    });
})(jQuery);
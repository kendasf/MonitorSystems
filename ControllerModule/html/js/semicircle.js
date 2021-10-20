/**
 * Semicircle extension for L.Circle.
 * Jan Pieter Waagmeester <jieter@jieter.nl>
 */

/*jshint browser:true, strict:false, globalstrict:false, indent:4, white:true, smarttabs:true*/
/*global L:true*/

function SetupSemicircle(L) {

        // save original getPathString function to draw a full circle.
        var original_getPathString = L.Circle.prototype.getPathString;

        L.Circle = L.Circle.extend({
                options: {
                        startAngle: 0,
                        stopAngle: 359.9999,
                        startLength: 0
                },

                // make sure 0 degrees is up (North) and convert to radians.
                _fixAngle: function (angle) {
                        return (angle - 90) * L.LatLng.DEG_TO_RAD;
                },
                startAngle: function () {
                        return this._fixAngle(this.options.startAngle);
                },
                stopAngle: function () {
                        return this._fixAngle(this.options.stopAngle);
                },
                startLength: function() {
                        var c = this._radius / this._mRadius;

                        return this.options.startLength * c;
                },

                //rotate point x,y+r around x,y with angle.
                rotated: function (angle, r) {
                        return this._point.add(
                                L.point(Math.cos(angle), Math.sin(angle)).multiplyBy(r)
                        ).round();
                },

                getPathString: function () {
                        var center = this._point,
                            r = this._radius,
                            r2 = this.startLength();

                        // If we want a circle, we use the original function
                        if (this.options.startAngle === 0 && this.options.stopAngle > 359) {
                                return original_getPathString.call(this);
                        }

                        var start_b = this.rotated(this.startAngle(), r);
                        var end_b = this.rotated(this.stopAngle(), r);

                        var start_a = this.rotated(this.startAngle(), this.startLength());
                        var end_a = this.rotated(this.stopAngle(), this.startLength());

                        if (this._checkIfEmpty()) {
                                return '';
                        }

                        if (L.Browser.svg) {
                                var largeArc = (this.options.stopAngle - this.options.startAngle >= 180) ? '1' : '0';
                                //move to center
                                var ret = "M" + start_a.x + "," + start_a.y;
                                //lineTo point on circle startangle from center
                                ret += "L " + start_b.x + "," + start_b.y;
                                //make circle from point start - end:
                                ret += "A " + r + "," + r + ",0," + largeArc + ",1," + end_b.x + "," + end_b.y;
                                //lineTo point on inner circle startangle from center
                                ret += "L " + end_a.x + "," + end_a.y;
                                // inner circle
                                ret += "A " + r2 + "," + r2 + ",0," + largeArc + ",0," + start_a.x + "," + start_a.y + " z";

                                return ret;
                        } else {
                                //TODO: fix this for semicircle...
                                center._round();
                                r = Math.round(r);
                                return "A " + center.x + "," + center.y + " " + r + "," + r + " 0," + (65535 * 360);
                        }
                },
                setStartAngle: function (angle) {
                        this.options.startAngle = angle;
                        return this.redraw();
                },
                setStopAngle: function (angle) {
                        this.options.stopAngle = angle;
                        return this.redraw();
                },
                setDirection: function (direction, degrees, start_length) {
                        if (degrees === undefined) {
                                degrees = 10;
                        }
                        this.options.startAngle = direction - (degrees / 2);
                        this.options.stopAngle = direction + (degrees / 2);
                        this.options.startLength = start_length;

                        return this.redraw();
                }
        });
        L.Circle.include(!L.Path.CANVAS ? {} : {
                _drawPath: function () {
                        var center = this._point,
                            r = this._radius;

                        var start_a = this.rotated(this.startAngle(), this.startLength());
                        var start_b = this.rotated(this.startAngle(), r);
                        var end = this.rotated(this.stopAngle(), this.startLength());

                        this._ctx.beginPath();
                        this._ctx.moveTo(start_a.x, start_a.y);
                        this._ctx.lineTo(start_b.x, start_b.y);
                        this._ctx.arc(center.x, center.y, this._radius,
                                this.startAngle(), this.stopAngle(), false);
                        this._ctx.lineTo(end.x, end.y);
                        this._ctx.arc(center.x, center.y, this._radius,
                                this.stopAngle(), this.startAngle(), false);

                        console.log(this.startLength());

/*
                        var start = this.rotated(this.startAngle(), r);

                        this._ctx.beginPath();
                        this._ctx.moveTo(center.x, center.y);
                        this._ctx.lineTo(start.x, start.y);

                        this._ctx.arc(center.x, center.y, this._radius,
                                this.startAngle(), this.stopAngle(), false);
                        this._ctx.lineTo(center.x, center.y);
                        */
                }

                // _containsPoint: function (p) {
                // TODO: fix for semicircle.
                // var center = this._point,
                //     w2 = this.options.stroke ? this.options.weight / 2 : 0;

                //  return (p.distanceTo(center) <= this._radius + w2);
                // }
        });
};
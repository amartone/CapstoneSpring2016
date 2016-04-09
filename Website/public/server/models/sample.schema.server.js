module.exports = function(mongoose) {

    // use mongoose to declare a user schema
    var sample = mongoose.Schema(
        {
            userId: String,
            measurements: [{impedance_magnitude: Number, impedance_phase: Number, pressure: Number}]
        }, {collection: 'capstone.sample'});
    return sample;
};


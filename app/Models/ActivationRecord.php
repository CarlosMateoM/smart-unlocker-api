<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;

class ActivationRecord extends Model
{
    /** @use HasFactory<\Database\Factories\ActivationRecordFactory> */
    use HasFactory;

    protected $fillable = [
        'user_id',
        'date',
        'time',
    ];

    


    public function user()
    {
        return $this->belongsTo(User::class);
    }
}

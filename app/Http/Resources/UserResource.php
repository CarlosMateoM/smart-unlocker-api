<?php

namespace App\Http\Resources;

use Illuminate\Http\Request;
use Illuminate\Http\Resources\Json\JsonResource;

class UserResource extends JsonResource
{
    /**
     * Transform the resource into an array.
     *
     * @return array<string, mixed>
     */
    public function toArray(Request $request): array
    {
        return [
            'id'                    => $this->id,
            'uid'                   => $this->uid,
            'name'                  => $this->name,
            'email'                 => $this->email,
            'role'                  => $this->role,
            'activation_records'    => ActivationRecordResource::collection($this->whenLoaded('activationRecords')),
        ];
    }
}
